#include "../include/servidor.h"
#include <iostream>
#include <boost/beast/core/detail/base64.hpp>

Servidor::Servidor(int port, rclcpp::Node::SharedPtr node)
    : acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), socket_(io_context_), nodeManager(node) {}

void Servidor::run()
{
    std::cout << "Servidor escuchando en el puerto " << acceptor_.local_endpoint().port() << std::endl;
    startAccept();
    io_context_.run();
}

void Servidor::startAccept()
{
    // Acepta una nueva conexión
    std::cout << "Waiting for a client to connect...\n";
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
                           {
            if (!ec) {
                std::cout << "Client connected.\n";
                startRead();
            } else {
                std::cerr << "Error accepting connection: " << ec.message() << "\n";
                startAccept(); // Retry accepting a connection
            } });
}

void Servidor::resetConnection()
{
    try
    {
        std::cout << "Cerrando la conexión con el cliente...\n";
        socket_.close();
        buf_.clear();  // Limpia el buffer
        startAccept(); // Espera por un nuevo cliente
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error cerrando el socket: " << e.what() << "\n";
    }
}

void Servidor::closeServer()
{
    std::cout << "Apagando el servidor...\n";

    try
    {
        // Detener el contexto de IO
        io_context_.stop();

        // Cerrar el socket
        if (socket_.is_open())
        {
            pri1("Estoy intentando cerrarme mucho");
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            socket_.close();
        }

        std::cout << "Servidor cerrado correctamente.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error cerrando el servidor: " << e.what() << "\n";
    }
}

void Servidor::startRead()
{
    socket_.async_read_some(
        boost::asio::buffer(buffer_array),
        [this](const boost::system::error_code &ec, long unsigned int bytes_transferred)
        {
            if (!ec)
            {
                std::string buffer_string(buffer_array.data(), bytes_transferred);
                std::vector<std::string> buffer_vector = splitJson(buffer_string);
                handleType(buffer_vector);
            }
            else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset)
            {
                std::cout << "Cliente desconectado.\n";
                resetConnection();
            }
            else
            {
                std::cerr << "Error en la lectura: " << ec.message() << "\n";
                resetConnection();
            }
        });
}

void Servidor::handleType(std::vector<std::string> const &jsons)
{
    for (auto json_ : jsons)
    {
        json json_msg = json::parse(json_);
        if (json_msg.contains("opt") && json_msg["opt"] == headerToString(MSG))
        {
            HandleMsg handleMsg(nodeManager);
            handleMsg.handleMsgJson(json_msg);
        }
        else if (json_msg.contains("opt") && json_msg["opt"] == headerToString(REQUEST_IMG))
            handleRequestImg(json_msg);
        else if (json_msg.contains("opt") && json_msg["opt"] == headerToString(REQUEST_MSG))
            handleRequestMsg(json_msg);
        else
            std::cout << "El campo 'opt' no contiene 'MSG' o 'IMG_REQUEST' o no existe.\n";
    }
    startRead();
}

// void Servidor::handleMsg(const json &json_msg)
// {
//     if (json_msg.contains("target") && json_msg["target"] == targetToString(Joystick_Position))
//         handleMsgJoystickPosition(json_msg);
//     else if (json_msg.contains("target") && json_msg["target"] == targetToString(State_Remote_Controlled))
//         handleMsgStateRemoteControlled(json_msg);
//     else if (json_msg.contains("target") && json_msg["target"] == targetToString(Delete_Map))
//         handleMsgDeleteMap(json_msg);
//     else if (json_msg.contains("target") && json_msg["target"] == targetToString(Change_Map_Name))
//         handleMsgChangeMapName(json_msg);
//     else if (json_msg.contains("target") && json_msg["target"] == targetToString(Save_Map))
//         handleMsgSaveMap(json_msg);
//     else if (json_msg.contains("target") && json_msg["target"] == targetToString(Goal_Pose))
//         handleMsgGoalPose(json_msg);
// }

void Servidor::handleRequestMsg(const json &json_msg)
{
    if (json_msg.contains("target") && json_msg["target"] == targetToString(Map_Name))
    {
        std::string path = PATH2MAP;
        sendMsg(toJson::sendMapName(getMapName(path)));
    }
}

void Servidor::handleRequestImg(const json &json_msg)
{
    if (json_msg.contains("target") && json_msg["target"] == targetToString(Request_Map_SLAM))
    {
        std::string path = PATH2MAP;
        if (json_msg.contains("map_name") && json_msg["map_name"] != "")
        {
            path += "/" + replaceSpaces(json_msg["map_name"]) + ".pgm";
            int width = 0, height = 0;
            getImageSize(path, width, height);
            sendMsg(toJson::sendInfoMap(json_msg["map_name"], width, height));
        }
        else
        {
            FinalPosition fp = nodeManager.getPositionRobotPixel(path + "/temporal_map.yaml");
            path += "/temporal_map.pgm";

            // send robot position pixels
            sendMsg(toJson::sendRobotPositionPixel(fp.x_pixel, fp.y_pixel, fp.yaw));
            nodeManager.refresh_map();
        }
        std::thread sendMapThread(&Servidor::sendImageMap, this, path);
        sendMapThread.detach();
    }
    else if (json_msg.contains("target") && json_msg["target"] == targetToString(Img_Map_Select))
    {
        std::string path = PATH2MAP;
        path += "/" + replaceSpaces(json_msg["map_name"]);

        // nodeManager.refresh_map(json_msg["map_name"]);
        std::thread sendMapThread(&Servidor::sendImageMap, this, path);
        sendMapThread.detach();
    }
}

// void Servidor::handleMsgJoystickPosition(const json &json_msg)
// {
//     float linear, angular;
//     nodeManager.create_publisher(stringToTarget(json_msg["target"]));
//     toJson::getPositionJoystick(json_msg, linear, angular); // json
//     pri1(std::to_string(linear) + "<-linear angular ->" + std::to_string(angular));
//     nodeManager.execute_position(linear, angular);
// }

// void Servidor::handleMsgStateRemoteControlled(const json &json_msg)
// {
//     if (!json_msg["in"])
//     {
//         // bkill the process (all)
//         nodeManager.close_publisher(Request_Map_SLAM);
//         nodeManager.close_publisher(Joystick_Position);
//         // SI HE INICIALIZADO ALGO CON UN ROS2 RUN O LAUNCH HAY QUE HACER UN KILL
//         // RVIZ
//         startRead();
//     }
//     else if (json_msg.contains("mapping") && json_msg["mapping"] == true)
//     {
//         nodeManager.create_publisher(Request_Map_SLAM);
//     }
//     nodeManager.create_publisher(Joystick_Position);
// }

// void Servidor::handleMsgDeleteMap(const json &json_msg)
// {
//     std::string path = PATH2MAP;
//     path += "/" + replaceSpaces(json_msg["map_name"]);
//     std::string path1 = path + ".pgm";
//     if (std::remove(path1.c_str()) == 0)
//         std::cout << "Archivo eliminado exitosamente: " << path << std::endl;
//     else
//         std::cerr << "Error al eliminar el archivo: " << path << std::endl;
//     std::string path2 = path + ".yaml";
//     if (std::remove(path2.c_str()) == 0)
//         std::cout << "Archivo eliminado exitosamente: " << path << std::endl;
//     else
//         std::cerr << "Error al eliminar el archivo: " << path << std::endl;
// }

// void Servidor::handleMsgChangeMapName(const json &json_msg)
// {
//     // if (fs::exists(nuevoNombre))
//     // {
//     //     std::cerr << "Error: El archivo \"" << nuevoNombre << "\" ya existe." << std::endl;
//     //     return 1; // Salir con código de error
//     // }

//     // // Intentar renombrar el archivo
//     // if (std::rename(nombreOriginal.c_str(), nuevoNombre.c_str()) == 0)
//     // {
//     //     std::cout << "El archivo se renombró exitosamente a: " << nuevoNombre << std::endl;
//     // }
//     // else
//     // {
//     //     std::cerr << "Error al renombrar el archivo: " << nombreOriginal << std::endl;
//     // }
// }

// void Servidor::handleMsgSaveMap(const json &json_msg)
// {
//     std::string path = PATH2MAP;
//     path += "/" + replaceSpaces(json_msg["map_name"]);
//     nodeManager.refresh_map(json_msg["map_name"]);
// }

// void Servidor::handleMsgGoalPose(const json &json_msg)
// {
// }

void Servidor::sendMsg(const json &json_msg)
{
    std::string jsonStr = json_msg.dump();
    pri1(jsonStr);
    boost::asio::write(socket_, boost::asio::buffer(jsonStr));
}

void Servidor::sendImageMap(const std::string &name_map)
{
    try
    {
        bool img_map_SLAM;
        if (name_map.find("temporal") != std::string::npos)
            img_map_SLAM = true;
        else
            img_map_SLAM = false;
        const std::size_t maxJsonSize = 2048; // Tamaño máximo por paquete

        // Leer imagen PGM usando OpenCV
        cv::Mat image = cv::imread(name_map, cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            throw std::runtime_error("Error al leer la imagen.");
        }

        // Comprimir imagen y convertirla a formato PNG
        std::vector<int> compression_params = {cv::IMWRITE_PNG_COMPRESSION, 9}; // Nivel de compresión (0-9)
        std::vector<uchar> compressed_image;
        cv::imencode(".png", image, compressed_image, compression_params);

        // Leer los datos comprimidos directamente desde la memoria
        std::size_t totalSize = compressed_image.size();
        const std::size_t maxDataSize = 1300; // Espacio reservado para los datos de la imagen
        std::size_t bytesSent = 0;
        std::size_t numFrame = 0;
        std::size_t totalFrames = (totalSize + maxDataSize - 1) / maxDataSize; // Calcular total de frames

        while (bytesSent < totalSize)
        {
            std::size_t bytesRead = std::min(maxDataSize, totalSize - bytesSent);
            std::vector<char> buffer(compressed_image.begin() + bytesSent, compressed_image.begin() + bytesSent + bytesRead);

            // Convertir datos a base64
            std::string hexData = toBase64(buffer.data(), bytesRead);

            // Crear el JSON
            nlohmann::json jsonMessage = toJson::sendImgMap(hexData, bytesRead, totalSize, numFrame, totalFrames, img_map_SLAM);

            // Serializar el JSON
            std::string jsonStr = jsonMessage.dump();

            // Verificar que el tamaño total no exceda el límite
            if (jsonStr.size() > maxJsonSize)
            {
                std::string str = "El JSON generado excede el tamaño máximo permitido " + std::to_string(jsonStr.size());
                throw std::runtime_error(str);
            }

            // Enviar el JSON por el socket
            pri1("IMAGEN ENVIADA: " + jsonStr);
            pri1(std::to_string(jsonStr.size()));
            boost::asio::write(socket_, boost::asio::buffer(jsonStr));
            bytesSent += bytesRead;
            numFrame++;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error en sendImageMap: " << e.what() << std::endl;
    }
}
