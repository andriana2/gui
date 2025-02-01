#include <opencv2/opencv.hpp>
#include <vector>

struct Punto {
    float x, y;
};

// Algoritmo de Corner Cutting para suavizar la trayectoria
std::vector<Punto> suavizarTrayectoriaCornerCutting(const std::vector<Punto>& puntos, int iteraciones, float alpha) {
    std::vector<Punto> resultado = puntos;

    for (int iter = 0; iter < iteraciones; ++iter) {
        std::vector<Punto> nuevaLista;

        for (size_t i = 0; i < resultado.size() - 1; ++i) {
            Punto p1 = resultado[i];
            Punto p2 = resultado[i + 1];

            // Punto más cerca de p1
            Punto q{
                (1 - alpha) * p1.x + alpha * p2.x,
                (1 - alpha) * p1.y + alpha * p2.y
            };

            // Punto más cerca de p2
            Punto r{
                alpha * p1.x + (1 - alpha) * p2.x,
                alpha * p1.y + (1 - alpha) * p2.y
            };

            nuevaLista.push_back(q);
            nuevaLista.push_back(r);
        }

        resultado = nuevaLista;
    }
    return resultado;
}

// Función para dibujar una trayectoria con un color específico
void dibujarTrayectoria(cv::Mat& imagen, const std::vector<Punto>& puntos, cv::Scalar color, int grosor) {
    for (size_t i = 1; i < puntos.size(); ++i) {
        cv::line(imagen, cv::Point(puntos[i - 1].x * 5, 500 - puntos[i - 1].y * 5),
                 cv::Point(puntos[i].x * 5, 500 - puntos[i].y * 5), color, grosor);
    }
}

int main() {
    std::vector<Punto> puntosOriginales = {
    {75, 12}, {73, 12}, {71, 13}, {70, 13}, {67, 13}, {66, 14}, {64, 14},
    {53, 20}, {51, 21}, {51, 21}, {50, 22}, {48, 24}, {48, 24}, {46, 26},
    {42, 31}, {41, 31}, {41, 32}, {40, 32}, {39, 33}, {39, 33}, {38, 34},
    {33, 36}, {33, 37}, {32, 37}, {32, 38}, {31, 39}, {31, 39}, {31, 40},
    {29, 46}, {29, 47}, {29, 48}, {29, 49}, {29, 51}, {29, 52}, {30, 54},
    {32, 65}, {32, 65}, {33, 67}, {33, 68}, {34, 70}, {34, 71}, {34, 71},
    {35, 77}, {36, 78}, {36, 80}, {37, 81}, {38, 84}, {38, 85}, {38, 87},
    {40, 91}, {40, 91}, {41, 92}, {42, 92}, {43, 93}, {44, 93}, {45, 93},
    {55, 94}, {57, 94}, {59, 94}, {60, 93}, {62, 93}, {64, 93}, {66, 93},
    {75, 94}, {76, 94}, {77, 94}, {78, 94}, {79, 94}, {79, 94}, {80, 94},
    {85, 91}, {86, 91}, {87, 91}, {87, 90}, {89, 89}, {91, 89}, {93, 88},
    {100, 83}, {101, 83}, {102, 81}, {102, 81}, {103, 80}, {103, 80},
    {106, 72}, {106, 71}, {104, 62}, {104, 61}, {104, 61}, {103, 61},
    {103, 61}, {103, 60}
};

    int iteraciones = 3;
    float alpha = 0.25; // Ajusta qué tan agresivo es el corte

    std::vector<Punto> puntosSuavizados = suavizarTrayectoriaCornerCutting(puntosOriginales, iteraciones, alpha);

    // Crear la imagen en negro
    cv::Mat imagen = cv::Mat::zeros(500, 500, CV_8UC3);

    // Dibujar la trayectoria original en rojo
    dibujarTrayectoria(imagen, puntosOriginales, cv::Scalar(0, 0, 255), 2); // Rojo

    // Dibujar la trayectoria suavizada en verde
    dibujarTrayectoria(imagen, puntosSuavizados, cv::Scalar(0, 255, 0), 2); // Verde

    // Mostrar la imagen con ambas trayectorias
    cv::imshow("Trayectoria Original (Rojo) vs Suavizada con Corner Cutting (Verde)", imagen);
    cv::waitKey(0);

    return 0;
}
