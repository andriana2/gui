#ifndef HEADER_H
#define HEADER_H

enum Info_Message
{
    Lista_mapa = 1,
    Position_joystick,
    Guardar_mapa,

};

#define PATH2MAP "/home/robogait/Desktop/gui_andri/gui/server/maps"

//#define MAP_SAVER_CLI "ros2 run nav2_map_server map_saver_cli -t /map -f " + PATH2MAP + " "
#define MAP_SAVER_CLI "ros2 run nav2_map_server map_saver_cli -t /map -f /home/robogait/Desktop/gui_andri/gui/server/maps"

// launch
#define RVIZ_LAUNCH_MAPING "ros2 launch turtlebot3_cartographer cartographer.launch.py &" // &ejecutar en segundo plano

// kill
#define RVIZ_LAUNCH_MAPING_KILL "pgrep -f 'ros2 launch turtlebot3_cartographer cartographer.launch.py'"

//    if (number == 10) // como matar procesos
//     {
//         if (!turtlesim_started_)
//         {
//             std::system("ros2 run turtlesim turtlesim_node &"); // Ejecuta turtlesim en segundo plano
//             turtlesim_started_ = true;
//         }
//     }
//     else if (number == 11)
//     {
//         if (turtlesim_started_)
//         {
//             std::system("pkill turtlesim_node"); // Detiene el nodo de turtlesim
//             turtlesim_started_ = false;
//         }
//     }

// topic
#define TF_TOPIC "/tf"
#define CMD_TOPIC "/cmd_vel"

// turtlesim
#define CMD_TOPIC_TURTLESIM "/turtle1/cmd_vel"

#endif // HEADER_H