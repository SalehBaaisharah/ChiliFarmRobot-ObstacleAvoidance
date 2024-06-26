#include <webots/robot.h>
#include <webots/range_finder.h>
#include <webots/motor.h>
#include <stdio.h>
#include <webots/distance_sensor.h>
#include <webots/gyro.h>

#define TIME_STEP 64

// Declare the motors
WbDeviceTag wheel1, wheel2, wheel3, wheel4;

// Declare the range finder sensors
WbDeviceTag right_finder, left_finder, front_finder;

// Declare the distance sensors
WbDeviceTag front_distance_sensor, left_distance_sensor, right_distance_sensor;

// Declare the gyro sensor
WbDeviceTag gyro;

// Declare the maximum speed of the motors
int max_speed = 10;

// Declare the threshold for the range finders
double obstacle_threshold = 0.5; // 0.5 meters
double obstacle_in_row_threshold = 0.2;

// Declare the counters
int front_obstacle_counter = 0;
int right_obstacle_counter = 0;
int left_obstacle_counter = 0;

// Define an array to store obstacle counts for each wheel diameter
int obstacle_counts[6];

// To move all the wheels forward
void move_forward() {
  wb_motor_set_velocity(wheel1, max_speed);
  wb_motor_set_velocity(wheel2, max_speed);
  wb_motor_set_velocity(wheel3, max_speed);
  wb_motor_set_velocity(wheel4, max_speed);
}

// To move all the wheels backward
void move_backward() {
  wb_motor_set_velocity(wheel1, -max_speed);
  wb_motor_set_velocity(wheel2, -max_speed);
  wb_motor_set_velocity(wheel3, -max_speed);
  wb_motor_set_velocity(wheel4, -max_speed);
  printf("Moving backward\n");
}

// To turn the robot right
void turn_right() {
  wb_motor_set_velocity(wheel1, -max_speed);
  wb_motor_set_velocity(wheel2, max_speed);
  wb_motor_set_velocity(wheel3, max_speed);
  wb_motor_set_velocity(wheel4, -max_speed);
  printf("Turning right\n");
}

// To turn the robot left
void turn_left() {
  wb_motor_set_velocity(wheel1, max_speed);
  wb_motor_set_velocity(wheel2, -max_speed);
  wb_motor_set_velocity(wheel3, -max_speed);
  wb_motor_set_velocity(wheel4, max_speed);
  printf("Turning left\n");
}

// To perform a U-turn
void performTurn(bool anti) {
  double initial_time = wb_robot_get_time();
  double turn_duration = 0.84; // Adjust this value as needed

  if (anti) {
    turn_left();
  } else {
    turn_right();
  }

  while (wb_robot_get_time() - initial_time < turn_duration) {
    wb_robot_step(TIME_STEP);
  }

  move_forward();
  wb_robot_step(TIME_STEP * 14);

  if (anti) {
    turn_left();
  } else {
    turn_right();
  }

  initial_time = wb_robot_get_time();
  while (wb_robot_get_time() - initial_time <= turn_duration) {
    wb_robot_step(TIME_STEP);
  }

  printf("U-turn\n");
}

// To stop all the motors
void halt_motors() {
  wb_motor_set_velocity(wheel1, 0);
  wb_motor_set_velocity(wheel2, 0);
  wb_motor_set_velocity(wheel3, 0);
  wb_motor_set_velocity(wheel4, 0);
  printf("Motors halted\n");
}

int main(int argc, char **argv) {
  wb_robot_init();

  wheel1 = wb_robot_get_device("wheel1");
  wheel2 = wb_robot_get_device("wheel2");
  wheel3 = wb_robot_get_device("wheel3");
  wheel4 = wb_robot_get_device("wheel4");

  wb_motor_set_position(wheel1, INFINITY);
  wb_motor_set_position(wheel2, INFINITY);
  wb_motor_set_position(wheel3, INFINITY);
  wb_motor_set_position(wheel4, INFINITY);

  right_finder = wb_robot_get_device("right_finder");
  left_finder = wb_robot_get_device("left_finder");
  front_finder = wb_robot_get_device("front_finder");

  wb_range_finder_enable(right_finder, TIME_STEP);
  wb_range_finder_enable(left_finder, TIME_STEP);
  wb_range_finder_enable(front_finder, TIME_STEP);
  printf("Range finders enabled\n");

  front_distance_sensor = wb_robot_get_device("front_distance_sensor");
  left_distance_sensor = wb_robot_get_device("left_distance_sensor");
  right_distance_sensor = wb_robot_get_device("right_distance_sensor");

  wb_distance_sensor_enable(front_distance_sensor, TIME_STEP);
  wb_distance_sensor_enable(left_distance_sensor, TIME_STEP);
  wb_distance_sensor_enable(right_distance_sensor, TIME_STEP);
  printf("Distance sensors enabled\n");

  gyro = wb_robot_get_device("gyro");

  wb_gyro_enable(gyro, TIME_STEP);
  printf("Gyro sensor enabled\n");

  typedef enum RobotState {
    MOVE_FORWARD,
    TURN_RIGHT,
    TURN_LEFT,
    AVOID_OBSTACLE_RIGHT,
    AVOID_OBSTACLE_LEFT,
    RETURN_TO_ROW_RIGHT,
    RETURN_TO_ROW_LEFT,
    U_TURN,
    STOP
  } RobotState;

  RobotState current_state = MOVE_FORWARD;

  // Declare a flag to perform anti u-turn
  bool performAntiUTurn = false;

  while (wb_robot_step(TIME_STEP) != -1) {

    front_obstacle_counter = 0;
    right_obstacle_counter = 0;
    left_obstacle_counter = 0;

    const float *right_range = wb_range_finder_get_range_image(right_finder);
    const float *left_range = wb_range_finder_get_range_image(left_finder);
    const float *front_range = wb_range_finder_get_range_image(front_finder);

    double front_distance = wb_distance_sensor_get_value(front_distance_sensor);
    double left_distance = wb_distance_sensor_get_value(left_distance_sensor);
    double right_distance = wb_distance_sensor_get_value(right_distance_sensor);


    // If an obstacle is detected in front, decide which way to avoid based on the left and right sensors
    if (left_distance < obstacle_in_row_threshold) {
        current_state = AVOID_OBSTACLE_RIGHT;
        printf("Obstacle detected on left side.\n");
    } else if (right_distance < obstacle_in_row_threshold) {
        current_state = AVOID_OBSTACLE_LEFT;
        printf("Obstacle detected on right side.\n");
    }

    bool obstacle_detected = false;
    int total_obstacles = 0;
    int total_avoided = 0;

    // Check if any of the range finders detect an obstacle
    if (*front_range <= obstacle_threshold || *right_range <= obstacle_threshold || *left_range <= obstacle_threshold) {
        obstacle_detected = true;
    }

    // Check if the robot has avoided the obstacle in the next step(s)
    // If an obstacle was detacted in the previous step(s) and there is no obstacle now, increment total avoided
    if (obstacle_detected && *front_range > obstacle_threshold && *right_range > obstacle_threshold && *left_range > obstacle_threshold) {
        obstacle_detected = false;
        total_avoided++;
    }

    // Increment the counters when an obstacle is detected
    if (*front_range <= obstacle_threshold) {
        front_obstacle_counter++;
    }
    if (*right_range <= obstacle_threshold) {
        right_obstacle_counter++;
    }
    if (*left_range <= obstacle_threshold) {
        left_obstacle_counter++;
    }

    switch (current_state) {
        case MOVE_FORWARD:
            move_forward();
            if (*front_range <= obstacle_threshold && *right_range <= obstacle_threshold && *left_range <= obstacle_threshold) {
                current_state = STOP;
                printf("Obstacles detected on all sides. Stopping.\n");
            } else if (*right_range <= obstacle_threshold && *front_range <= obstacle_threshold) {
                current_state = TURN_LEFT;
                printf("Obstacle detected on the right. Turning left.\n");
            } else if (*left_range <= obstacle_threshold && *front_range <= obstacle_threshold) {
                current_state = TURN_RIGHT;
                printf("Obstacle detected on the left. Turning right.\n");
            } else if (*front_range <= obstacle_threshold) {
                current_state = U_TURN;
                printf("End of farm reached. Performing U-turn.\n");
            } else {
                // No condition met, continue moving forward
            }
            break;

        case TURN_RIGHT:
            turn_right();
            wb_robot_step(TIME_STEP * 3);  
            current_state = MOVE_FORWARD;
            printf("Turn completed. Moving forward.\n");
            break;

        case TURN_LEFT:
            turn_left();
            wb_robot_step(TIME_STEP * 3);  
            current_state = MOVE_FORWARD;
            printf("Turn completed. Moving forward.\n");
            break;

        case AVOID_OBSTACLE_RIGHT:
            // To avoid obstacle by moving slightly to the right
            turn_right();
            wb_robot_step(TIME_STEP * 7);  
            move_forward();
            wb_robot_step(TIME_STEP * 3);
            current_state = RETURN_TO_ROW_LEFT;
            break;

        case AVOID_OBSTACLE_LEFT:
            // To avoid obstacle by moving slightly to the left
            turn_left();
            wb_robot_step(TIME_STEP * 7);  
            move_forward();
            wb_robot_step(TIME_STEP * 3);
            current_state = RETURN_TO_ROW_RIGHT;
            break;

        case RETURN_TO_ROW_RIGHT:
            // To return to the original row from the right side
            turn_right();
            wb_robot_step(TIME_STEP * 7);  
            current_state = MOVE_FORWARD;
            break;

        case RETURN_TO_ROW_LEFT:
            // To return to the original row from the left side
            turn_left();
            wb_robot_step(TIME_STEP * 7); 
            current_state = MOVE_FORWARD;
            break;

        case U_TURN:
            performTurn(performAntiUTurn); 
            performAntiUTurn = !performAntiUTurn;
            current_state = MOVE_FORWARD;
            printf("U-turn completed. Moving forward.\n");
            break;
        case STOP:
            halt_motors();
            break;
    }
}

// Cleanup
wb_robot_cleanup();
return 0;
