#include <Logger.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <vulkan_renderer.hpp>

void VulkanRender::execScene(uint32_t sceneIdx) {
  if (!sceneMap.contains(sceneIdx)) {
    sceneIdx = 0;
  }

  Logger::log(Logger::LogLevel::INFO,
              "Executing scene " + std::to_string(sceneIdx) + ".");
  sceneMap[sceneIdx]();
}

void VulkanRender::initializeScene() {

  const float scale = 640.f;
  const float overlap = 0.001 / scale;

  // Cena 0
  auto addScene0 = [=, this]() {
    // Floor
    this->compute.planes.push_back({
        glm::vec4(0.f, -0.5f, 0.f, 1.f / scale) * scale,
        glm::vec4(1.f, 0.f, 0.f, 0.f),
        glm::vec4(0.f, 0.f, -1.f, 0.f),
        glm::vec4(1.f + overlap, 2.f + overlap, 0.f, 0.f) * scale,
        {{0.47f, 0.17f, 0.10f, 1.0f},
         {0.6f, 0.6f, 0.6f, 1.0},
         {0., 0., 0., 0.}},
    });
    // Ceiling
    this->compute.planes.push_back(

        {glm::vec4(0.f, 0.5f, 0.f, 1.f / scale) * scale,
         glm::vec4(1.f, 0.f, 0.f, 0.f),
         glm::vec4(0.f, 0.f, 1.f, 0.f),
         glm::vec4(1.f + overlap, 2.f + overlap, 0.f, 0.f) * scale,
         {{0.6f, 0.6f, 0.6f, 1.0f}, {0., 0., 0., 0.}, {0.0, 0., 0., 0.}}});

    // Back wall
    this->compute.planes.push_back(
        {glm::vec4(0.f, 0.f, -1.f, 1.f / scale) * scale,
         glm::vec4(1.f, 0.f, 0.f, 0.f),
         glm::vec4(0.f, 1.f, 0.f, 0.f),
         glm::vec4(1.f + overlap, 1.f + overlap, 0.f, 0.f) * scale,
         {{0.6f, 0.6f, 0.6f, 1.0f}, {0., 0., 0., 0.}, {0.0, 0., 0., 0.}}});

    // Left wall
    this->compute.planes.push_back(
        {glm::vec4(-0.5f, 0.f, 0.f, 1.f / scale) * scale,
         glm::vec4(0.f, 0.f, 1.f, 0.f),
         glm::vec4(0.f, -1.f, 0.f, 0.f),
         glm::vec4(1280.f + overlap, 640.f + overlap, 0.f, 0.f) * scale,
         {{0.14453125, 0.3359375, 0.9296875, 1.0f},
          {0., 0., 0., 0.},
          {0.0, .0, .0, .0}}});

    // Right wall
    this->compute.planes.push_back(
        {glm::vec4(0.5f, 0.f, 0.f, 1.f / scale) * scale,
         glm::vec4(0.f, 0.f, 1.f, 0.f),
         glm::vec4(0.f, 1.f, 0.f, 0.f),
         glm::vec4(2.f + overlap, 1.f + overlap, 0.f, 0.f) * scale,
         {{0.1f, 0.4f, 0.f, 1.0f}, {0., 0., 0., .3}, {0.0, .0, .0, .0}}});

    // Front wall
    this->compute.planes.push_back(
        {glm::vec4(0.f, 0.f, 1.f, 1.f / scale) * scale,
         glm::vec4(1.f, 0.f, 0.f, 0.f),
         glm::vec4(0.f, -1.f, 0.f, 0.f),
         glm::vec4(1.f + overlap, 1.f + overlap, 0.f, 0.f) * scale,
         {{0.6f, 0.6f, 0.6f, 1.0f}, {0., 0., 0., 0.}, {0.0, .0, .0, .0}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.35f, -0.15f, -0.8f, 0.125f) * scale,
         {{0.4f, 0.4f, 0.4f, 1.f}, {0., 0., 0., 0.2}, {1.0, .0, .0, .0}}});

    this->compute.spheres.push_back(
        {glm::vec4(0.f, 0.f, -0.9f, 0.25f / 2.f) * scale,
         {{0.0f, 0.8f, 0.8f, 1.0f}, {0., 0., 0., 0.}, {0.0, .0, .0, .0}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.2f, -0.3f, -0.3f, 0.4f / 2.f) * scale,
         {{0.4f, 0.4f, 0.4f, 0.66f},
          {0., 0., 0., 0.2},
          {2.0, -1.f, -1.f, 0.f}}});

    // Plane light(luz do teto)
    this->compute.planes.push_back(
        {glm::vec4(0.f, 0.49f, 0.f, 1.f / scale) * scale,
         glm::vec4(1.f, 0.f, 0.f, 0.f),
         glm::vec4(0.f, 0.f, 1.f, 0.f),
         glm::vec4(0.5f, 1.f, 0.f, 0.f) * scale,
         {{1.f, 1.f, 1.f, 1.0f}, {10.f, 10.f, 10.f, 0.0f}, {4.0, .0, .0, .0}}});
  };

  // Cena 1
  auto addScene1 = [=, this]() {
    // Floor
    this->compute.planes.push_back({
        glm::vec4(0.f, -0.56f, 0.f, 1.f / scale) * scale,
        glm::vec4(1.f, 0.f, 0.f, 0.f),
        glm::vec4(0.f, 0.f, -1.f, 0.f),
        glm::vec4(1.f + overlap, 2.f + overlap, 0.f, 0.f) * scale,
        {{0.47f, 0.47f, 0.47f, 1.0f},
         {0.6f, 0.6f, 0.6f, 1.0},
         {0., 0., 0., 0.}},
    });

    float angle = glm::radians(45.);
    glm::mat4 rot =
        glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.f, 0.f, 0.f));
    // Plane light(luz do teto)
    this->compute.planes.push_back(
        {glm::vec4(0.f, -0.25f, -0.49f, 1.f / scale) * scale,
         rot*glm::vec4(1.f, 0.f, 0.f, 0.f),
         rot*glm::vec4(0.f, 1.f, 0.f, 0.f),
         glm::vec4(0.3f, 0.3f, 0.f, 0.f) * scale,
         {{0.f, 0.f, 1.f, 1.0f}, {0.f, 0.f, 10.f, 0.0f}, {4.0, .0, .0, .0}}});

    this->compute.spheres.push_back(
        {glm::vec4(0.f, -0.35f, -0.3f, 0.4f / 2.f) * scale,
         {{0.4f, 0.4f, 0.4f, 0.75f},
          {0., 0., 0., 0.2},
          {2.0, 0.f, -1.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(0.120817f, -0.383496f, -0.29f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {2.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.11f, -0.38f, -0.31f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {2.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.083441f, -0.405164f, -0.357282f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {1.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(0.035886f, -0.432239f, -0.331584f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {2.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(0.081815f, -0.388619f, -0.207217, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {1.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.023792f, -0.427971f, -0.266128f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.7f, 0.66f}, {0., 0., 0.7f, 0.}, {2.0, 0.f, 0.f, 0.f}}});

    this->compute.spheres.push_back(
        {glm::vec4(-0.066406f, -0.369171f, -0.217973f, 0.05f / 2.f) * scale,
         {{0.f, 0.f, 0.47f, 0.66f}, {0., 0., 0.7f, 0.}, {1.0, 0.f, 0.f, 0.f}}});
  };

  // Initialize camera
  float viewportWidth = (float)chain.extent.width;
  float fov_deg = 60.0f;
  float fov_rad = glm::radians(fov_deg);
  float vp_dist = (viewportWidth * 0.5f) / tan(fov_rad * 0.5f) / scale;

  auto addCamera0 = [=, this]() {
    // Default scene0

    glm::vec3 posDelta(0.);
    glm::vec3 atDelta(0.);

    glm::vec3 pos((glm::vec3(0.f, 0.f, vp_dist) + posDelta) * scale);
    glm::vec3 foward((glm::vec3(0.f, 0.f, -1.f) + atDelta) * scale);
    glm::vec3 up(0.f, 1.f, 0.f);

    this->compute.cameraUniform.pos = glm::vec4(pos, 1.0f);
    this->compute.cameraUniform.view = glm::lookAt(pos, foward, up);
    this->compute.cameraUniform.invView =
        glm::inverse(this->compute.cameraUniform.view);
  };

  auto addCamera1 = [=, this]() {
    // Caustic top right
    glm::vec3 posDelta(0.4f, 0.4f, -0.4f);
    glm::vec3 atDelta(-0.2f, -0.7f, -0.2f);

    glm::vec3 pos((glm::vec3(0.f, 0.f, vp_dist) + posDelta) * scale);
    glm::vec3 foward((glm::vec3(0.f, 0.f, -1.f) + atDelta) * scale);
    glm::vec3 up(0.f, 1.f, 0.f);

    compute.cameraUniform.pos = glm::vec4(pos, 1.0f);
    compute.cameraUniform.view = glm::lookAt(pos, foward, up);
    compute.cameraUniform.invView = glm::inverse(compute.cameraUniform.view);
  };

  auto addCamera2 = [=, this]() {
    // Caustic center back
    glm::vec3 posDelta(0.f, 0.f, 0.f);
    glm::vec3 atDelta(-0.3f, -0.7f, -0.2f);

    glm::vec3 pos((glm::vec3(0.f, 0.f, vp_dist) + posDelta) * scale);
    glm::vec3 foward((glm::vec3(0.f, 0.f, -1.f) + atDelta) * scale);
    glm::vec3 up(0.f, 1.f, 0.f);

    compute.cameraUniform.pos = glm::vec4(pos, 1.0f);
    compute.cameraUniform.view = glm::lookAt(pos, foward, up);
    compute.cameraUniform.invView = glm::inverse(compute.cameraUniform.view);
  };

  auto addCamera3 = [=, this]() {
    // Caustic center floor
    //glm::vec3 posDelta(0.f, -0.2f, -0.2f);
    //glm::vec3 atDelta(-0.1f, -0.5f, 0.f);
    //

    glm::vec3 posDelta(0.f, -0.1f, -0.1f);
    glm::vec3 atDelta(0.f, -1.f, 0.f);

    glm::vec3 pos((glm::vec3(0.f, 0.f, vp_dist) + posDelta) * scale);
    glm::vec3 foward((glm::vec3(0.f, 0.f, -1.f) + atDelta) * scale);
    glm::vec3 up(0.f, 1.f, 0.f);

    compute.cameraUniform.pos = glm::vec4(pos, 1.0f);
    compute.cameraUniform.view = glm::lookAt(pos, foward, up);
    compute.cameraUniform.invView = glm::inverse(compute.cameraUniform.view);
  };


  sceneMap[0] = [=, this]() {
    addCamera0();
    addScene0();
  };
  sceneMap[1] = [=, this]() {
    addCamera1();
    addScene0();
  };
  sceneMap[2] = [=, this]() {
    addCamera2();
    addScene0();
  };

  sceneMap[3] = [=, this]() {
    addCamera3();
    addScene1();
  };
}
