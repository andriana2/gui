#ifndef TOJSON_H
#define TOJSON_H

#include <iostream>
#include <nlohmann/json.hpp> // Asegúrate de incluir la biblioteca
#include "utils.h"

using json = nlohmann::json;

json sendRobotPositionPixel(int x, int y, float yaw);

json sendImgMapSlam(const std::string data, int size_data, int total_size_img, int num_frame, int total_frame);

void getPositionJoystick(json const &j, float &linear_output, float &angular_output);

json sendMapName(std::vector<std::string> const &vec_map_name);



#endif // TOJSON_H