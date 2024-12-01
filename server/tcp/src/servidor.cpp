#include "../include/servidor.h"
#include <iostream>

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
                readHeader();
            } else {
                std::cerr << "Error accepting connection: " << ec.message() << "\n";
                startAccept(); // Retry accepting a connection
            } });
}

void Servidor::readHeader()
{
    boost::asio::async_read_until(socket_, buffer_, ':',
                                  [this](boost::system::error_code ec, std::size_t bytes_transferred)
                                  {
                                      if (!ec)
                                      {
                                          //   std::cout << "el buffer es:" << std::endl;
                                          //   std::cout << buf_ << std::endl;

                                          std::istream stream(&buffer_);
                                          std::string tipo;
                                          std::getline(stream, tipo, ':'); // Lee hasta ':'

                                          std::string longitudStr;
                                          std::getline(stream, longitudStr, ':'); // Remove processed data

                                          int longitud = std::stoi(longitudStr);
                                          std::cout << "Tipo: " << tipo << ", Longitud: " << longitud << std::endl;

                                          std::string contenido;
                                          std::getline(stream, contenido, '\n'); // Remove processed data
                                          handleType(tipo, longitud, contenido);
                                      }
                                      else
                                      {
                                          handleDisconnect(ec);
                                      }
                                  });
}

void Servidor::handleType(std::string const &type, int const &size, std::string const &target)
{
    if (type == "MSG")
    {
        nodeManager.create_publisher(target);
        readBodyMsg(size, target);
    }
    else if (type == "IMG")
    {
        readBodyImage(size);
    }
}

void Servidor::readBodyMsg(size_t const &size, std::string const &target)
{
    std::string data;
    boost::asio::async_read(socket_, boost::asio::dynamic_buffer(data), boost::asio::transfer_exactly(size),
                            [this, &data, &target](boost::system::error_code ec, std::size_t bytes_transferred)
                            {
                                if (!ec)
                                {
                                    processMsg(data, target);
                                    readHeader();
                                }
                                else
                                {
                                    handleDisconnect(ec);
                                }
                            });
}
void Servidor::processMsg(std::string const &data, std::string const &target)
{
    if (target == get_info_message(Position_joystick))
    {
        float linear, angular;
        getValuePositionJoystick(data, linear, angular);
        nodeManager.execute_position(linear, angular);
    }
}

void Servidor::readBodyImage(size_t const &size)
{
    std::vector<uint8_t> data(size);
    boost::asio::async_read(socket_, boost::asio::dynamic_buffer(data), boost::asio::transfer_exactly(size),
                            [this, &data](boost::system::error_code ec, std::size_t bytes_transferred)
                            {
                                if (!ec)
                                {
                                    saveImage(data);
                                    readHeader();
                                }
                                else
                                {
                                    handleDisconnect(ec);
                                }
                            });
}

void Servidor::handleDisconnect(const boost::system::error_code &ec)
{
    if (ec == boost::asio::error::eof)
    {
        std::cout << "Client disconnected.\n";
    }
    else
    {
        std::cerr << "Error: " << ec.message() << "\n";
    }
    resetConnection();
}

void Servidor::saveImage(const std::vector<uint8_t> &buffer)
{
    // Abrir el archivo para guardar los datos binarios
    std::ofstream archivoSalida("imagen_recibida.jpg", std::ios::binary);
    if (!archivoSalida)
    {
        std::cerr << "Error al abrir el archivo para escribir." << std::endl;
        return;
    }

    // Escribir los datos de la imagen en el archivo
    archivoSalida.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    if (archivoSalida)
    {
        std::cout << "Imagen guardada con éxito." << std::endl;
    }
    else
    {
        std::cerr << "Error al escribir los datos en el archivo." << std::endl;
    }
}

void Servidor::resetConnection()
{
    socket_.close(); // Ensure the socket is closed
    buf_.clear();    // Clear the buffer
    startAccept();   // Wait for the next client
}

// void Servidor::enviarMensaje(const std::string &mensaje)
// {
//     std::string header = "SRV_MSG:" + std::to_string(mensaje.size()) + ":";
//     boost::asio::write(socket_, boost::asio::buffer(header + mensaje));
// }

// void Servidor::enviarImagen(const std::vector<uint8_t> &imagen)
// {
//     std::string header = "SRV_IMG:" + std::to_string(imagen.size()) + ":";
//     boost::asio::write(socket_, boost::asio::buffer(header));
//     boost::asio::write(socket_, boost::asio::buffer(imagen));
// }
