glm::mat4 MakeViewProjectionLookInDirection(glm::vec3 Pos, float Yaw, float Pitch, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
	glm::mat4 Proj, View;

	Proj = glm::perspective(FOVy, Ar, nearPlane, farPlane);

	Proj[1][1] *= -1;

	glm::mat4 Trasl = glm::translate(glm::mat4(1), Pos);
	glm::mat4 Rx, Ry, Rz;

	Rx = glm::rotate(glm::mat4(1), Pitch, glm::vec3(1, 0, 0));
	Ry = glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0));
	Rz = glm::rotate(glm::mat4(1), Roll, glm::vec3(0, 0, 1));

        View = glm::inverse(Trasl * Rz * Rx * Ry);

	glm::mat4 M = Proj * View;

	return M;
}

glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target, glm::vec3 Up, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
	glm::mat4 Proj, View;

	Proj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
	Proj[1][1] *= -1;
	View = glm::rotate(glm::mat4(1), -Roll, glm::vec3(0, 0, 1.0f)) * glm::lookAt(Pos, Target, Up);

	return Proj * View;
}

glm::mat4 MakeWorld(glm::vec3 Pos, float Yaw, float Pitch, float Roll) {
	glm::mat4 Trasl = glm::translate(glm::mat4(1), Pos);
	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 Rx, Ry, Rz;

	Rx = glm::rotate(glm::mat4(1), Pitch, glm::vec3(1, 0, 0));
	Ry = glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0));
	Rz = glm::rotate(glm::mat4(1), Roll, glm::vec3(0, 0, 1));

	return M = Trasl * Rz * Rx * Ry;
}
