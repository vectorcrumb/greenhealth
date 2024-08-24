#ifndef ROVER_DEFS_HPP_
#define ROVER_DEFS_HPP_

enum RoverAction {
  NONE = 0,
  STANDBY = 1,
  MOVE_MM = 2,
  MOVE_TO_NEXT_NODE = 3,
  MOVE_N_NODES = 4,
};

struct CurrentAction {
  RoverAction action;
  int parameter;
  int timeout_ms;
};

#endif
