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
        nodeManager.reset();
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
            pri1("Estoy intentando cerrarme");
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
    pri1("estoy en StartRead");
    socket_.async_read_some(
        boost::asio::buffer(buffer_array),
        [this](const boost::system::error_code &ec, long unsigned int bytes_transferred)
        {
            if (!ec)
            {
                pri1("Hola1");
                std::string buffer_string(buffer_array.data(), bytes_transferred);
                pri1("Hola2");
                std::vector<std::string> buffer_vector = splitJson(buffer_string);
                pri1("Hola3");
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
    pri1("Estoy en handleRequestImg");
    if (json_msg.contains("target") && json_msg["target"] == targetToString(Request_Map_SLAM))
    {
        std::string path = PATH2MAP;
        if (json_msg.contains("map_name") && json_msg["map_name"] != "")
        {
            std::cout << "+++++++++++++++Mapa seleccionado: " << json_msg["map_name"] << std::endl;
            float resolution = 0.0;
            getResolution(path + "/" + replaceSpaces(json_msg["map_name"]) + ".yaml", resolution);
            path += "/" + replaceSpaces(json_msg["map_name"]) + ".pgm";
            int width = 0, height = 0;
            getImageSize(path, width, height);
            
            sendMsg(toJson::sendInfoMap(json_msg["map_name"], width, height, resolution));
            std::thread sendMapThread(&Servidor::sendImageMap, this, path, false);
            sendMapThread.detach();
        }
        else
        {
            pri1("Estoy en else de handleRequestImg");
            FinalPosition fp = nodeManager.getPositionRobotPixel(path + "/temporal_map.yaml");
            path += "/temporal_map.pgm";

            // send robot position pixels
            pri1("Hola4");
            sendMsg(toJson::sendRobotPositionPixel(fp.x_pixel, fp.y_pixel, fp.yaw));
            pri1("Hola5");
            nodeManager.refresh_map();
            pri1("Hola6");
            std::thread sendMapThread(&Servidor::sendImageMap, this, path, true);
            sendMapThread.detach();
        }
    }
    else if (json_msg.contains("target") && json_msg["target"] == targetToString(Img_Map_Select))
    {
        std::string path = PATH2MAP;
        path += "/" + replaceSpaces(json_msg["map_name"]);

        // nodeManager.refresh_map(json_msg["map_name"]);
        std::thread sendMapThread(&Servidor::sendImageMap, this, path, false);
        sendMapThread.detach();
    }
}

void Servidor::sendMsg(const json &json_msg)
{
    std::string jsonStr = json_msg.dump();
    pri1(jsonStr);
    boost::asio::write(socket_, boost::asio::buffer(jsonStr));
}

void Servidor::sendImageMap(const std::string &name_map, bool img_map_SLAM)
{
    try
    {
        // bool img_map_SLAM;
        // if (name_map.find("temporal") != std::string::npos)
        //     img_map_SLAM = true;
        // else
        //     img_map_SLAM = false;
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
