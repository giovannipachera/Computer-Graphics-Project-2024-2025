#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline glm::mat4 MakeViewProjectionLookInDirection(glm::vec3 Pos, float Yaw,
                                                   float Pitch, float Roll,
                                                   float FOVy, float Ar,
                                                   float nearPlane,
                                                   float farPlane) {
  glm::mat4 Proj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
  Proj[1][1] *= -1;

  glm::mat4 Trasl = glm::translate(glm::mat4(1), Pos);
  glm::mat4 Rx = glm::rotate(glm::mat4(1), Pitch, glm::vec3(1, 0, 0));
  glm::mat4 Ry = glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0));
  glm::mat4 Rz = glm::rotate(glm::mat4(1), Roll, glm::vec3(0, 0, 1));

  glm::mat4 View = glm::inverse(Trasl * Rz * Rx * Ry);
  return Proj * View;
}

inline glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target,
                                          glm::vec3 Up, float Roll, float FOVy,
                                          float Ar, float nearPlane,
                                          float farPlane) {
  glm::mat4 Proj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
  Proj[1][1] *= -1;

  glm::mat4 View = glm::rotate(glm::mat4(1), -Roll, glm::vec3(0, 0, 1.0f)) *
                   glm::lookAt(Pos, Target, Up);

  return Proj * View;
}

inline glm::mat4 MakeWorld(glm::vec3 Pos, float Yaw, float Pitch, float Roll) {
  glm::mat4 Trasl = glm::translate(glm::mat4(1), Pos);

  glm::mat4 Rx = glm::rotate(glm::mat4(1), Pitch, glm::vec3(1, 0, 0));
  glm::mat4 Ry = glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0));
  glm::mat4 Rz = glm::rotate(glm::mat4(1), Roll, glm::vec3(0, 0, 1));

  return Trasl * Rz * Rx * Ry;
}
