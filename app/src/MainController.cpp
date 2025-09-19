//
// Created by boce on 7/26/25.
//

#include "engine/graphics/GraphicsController.hpp"
#include "engine/graphics/OpenGL.hpp"
#include "engine/platform/PlatformController.hpp"
#include "engine/resources/ResourcesController.hpp"

#include <MainController.hpp>
#include <spdlog/spdlog.h>

namespace app {

void MainPlatformEventObserver::on_key(engine::platform::Key key) { spdlog::info("Keyboard event: key={}, state={}", key.name(), key.state_str()); }

void MainPlatformEventObserver::on_mouse_move(engine::platform::MousePosition position) { spdlog::info("MousePosition: {} {}", position.x, position.y); }


void MainController::initialize() {
    spdlog::info("Main controller successfully initialized");
    auto observer = std::make_unique<MainPlatformEventObserver>();
    engine::core::Controller::get<engine::platform::PlatformController>()->register_platform_event_observer(std::move(observer));

    create_plane();
    engine::graphics::OpenGL::enable_depth_testing();
    engine::core::Controller::get<engine::graphics::GraphicsController>()->camera()->Position = glm::vec3(0.0f, 0.0f, 5.0f);
    set_instanced_tree();
    set_targets();
    set_crosshair();
    set_dirlight();
    set_spotlight();
    set_rifle_dirlight();
}

void MainController::poll_events() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_F1).state() == engine::platform::Key::State::JustPressed) {
        m_cursor_enable = !m_cursor_enable;
        platform->set_enable_cursor(m_cursor_enable);
    }

    if (platform->key(engine::platform::KEY_P).state() == engine::platform::Key::State::JustPressed) {
        awake_targets();
        create_boundingbox_targets();
    }
}

bool MainController::loop() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_ESCAPE).state() == engine::platform::Key::State::JustPressed) { return false; }
    return true;
}

void MainController::update() {
    update_fps_camera();
    update_speed();
    update_jump();
    update_spotlight();
    update_raycast();
    update_targets();
    check_boundingbox_intersects();
}

void MainController::update_raycast() {
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    m_raycast.origin = camera->Position;
    m_raycast.dir = camera->Front;
}


void MainController::update_camera() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    float dt = platform->dt();

    if (platform->key(engine::platform::KEY_W).state() == engine::platform::Key::State::Pressed) { camera->move_camera(engine::graphics::Camera::Movement::FORWARD, dt); }

    if (platform->key(engine::platform::KEY_S).state() == engine::platform::Key::State::Pressed) { camera->move_camera(engine::graphics::Camera::Movement::BACKWARD, dt); }

    if (platform->key(engine::platform::KEY_A).state() == engine::platform::Key::State::Pressed) { camera->move_camera(engine::graphics::Camera::Movement::LEFT, dt); }

    if (platform->key(engine::platform::KEY_D).state() == engine::platform::Key::State::Pressed) { camera->move_camera(engine::graphics::Camera::Movement::RIGHT, dt); }

    auto mouse = platform->mouse();
    camera->rotate_camera(mouse.dx, mouse.dy);
}

void MainController::update_fps_camera() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    float dt = platform->dt();

    camera->move_fps_camera(
            platform->key(engine::platform::KEY_W).state() == engine::platform::Key::State::Pressed,
            platform->key(engine::platform::KEY_S).state() == engine::platform::Key::State::Pressed,
            platform->key(engine::platform::KEY_D).state() == engine::platform::Key::State::Pressed,
            platform->key(engine::platform::KEY_A).state() == engine::platform::Key::State::Pressed,
            dt
            );

    auto mouse = platform->mouse();
    camera->rotate_camera(mouse.dx, mouse.dy);
}

void MainController::update_speed() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();

    if (platform->key(engine::platform::KEY_LEFT_SHIFT).state() == engine::platform::Key::State::JustPressed) { camera->MovementSpeed = engine::graphics::Camera::RUN; }

    if (platform->key(engine::platform::KEY_LEFT_SHIFT).state() == engine::platform::Key::State::JustReleased) { camera->MovementSpeed = engine::graphics::Camera::WALK; }
}

void MainController::update_jump() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();

    if (!camera->Jump && platform->key(engine::platform::KEY_SPACE).state() == engine::platform::Key::State::JustPressed) { camera->Jump = true; }

    camera->update_jump(platform->dt());
}

void MainController::set_dirlight() {
    m_dirlight.direction = glm::vec3(-0.5f, -2.5f, -5.0f);
    m_dirlight.ambient = glm::vec3(0.08f, 0.08f, 0.08f);
    m_dirlight.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    m_dirlight.specular = glm::vec3(0.5f, 0.5f, 0.5f);
}

void MainController::set_rifle_dirlight() {
    m_rifle_dirlight.direction = glm::vec3(0.5f, -0.7f, -0.3f);
    m_rifle_dirlight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    m_rifle_dirlight.diffuse = glm::vec3(0.55f, 0.55f, 0.55f);
    m_rifle_dirlight.specular = glm::vec3(0.5f, 0.5f, 0.5f);
}


void MainController::set_spotlight() {
    m_spotlight.ambient = glm::vec3(0.0f);
    m_spotlight.diffuse = glm::vec3(0.6f);
    m_spotlight.specular = glm::vec3(0.6f);
    m_spotlight.inner_cut_off = glm::cos(glm::radians(10.5f));
    m_spotlight.outer_cut_off = glm::cos(glm::radians(12.0f));
    m_spotlight.constant = 1.0f;
    m_spotlight.linear = 0.22f;
    m_spotlight.quadratic = 0.20f;
}

void MainController::update_spotlight() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();

    m_spotlight.position = camera->Position;
    m_spotlight.direction = camera->Front;

    if (platform->key(engine::platform::KEY_V).state() == engine::platform::Key::State::JustPressed) { m_spotlight.lamp_on = 1 - m_spotlight.lamp_on; }

    if (platform->key(engine::platform::KEY_1).state() == engine::platform::Key::State::JustPressed) {
        m_spotlight.diffuse = glm::vec3(0.6f);
        m_spotlight.specular = glm::vec3(0.6f);
        m_spotlight.linear = 0.22f;
        m_spotlight.quadratic = 0.20f;
    }

    if (platform->key(engine::platform::KEY_2).state() == engine::platform::Key::State::JustPressed) {
        m_spotlight.diffuse = glm::vec3(1.0f);
        m_spotlight.specular = glm::vec3(1.0f);
        m_spotlight.linear = 0.09f;
        m_spotlight.quadratic = 0.032f;
    }
}

void MainController::begin_draw() { engine::graphics::OpenGL::clear_buffers(); }

void MainController::draw() {
    draw_instanced_tree();
    draw_plane();
    draw_cabin();
    draw_targets();
    draw_rifle();
    draw_skybox();
    draw_crosshair();
}

void MainController::end_draw() { engine::core::Controller::get<engine::platform::PlatformController>()->swap_buffers(); }


void MainController::terminate() { delete m_model_tree; }


void MainController::set_targets() {
    auto model = engine::core::Controller::get<engine::resources::ResourcesController>()->model("target");
    m_targets.emplace_back(model, glm::vec3(-0.2f, -0.5f, 0.3f));
    m_targets.emplace_back(model, glm::vec3(2.0f, -0.5f, 1.2f));
    m_targets.emplace_back(model, glm::vec3(0.6f, -0.5f, 2.0f));
    m_targets.emplace_back(model, glm::vec3(-1.2f, -0.5f, -1.0f));
    m_targets.emplace_back(model, glm::vec3(1.3f, -0.5f, -1.5f));
}

void MainController::draw_targets() {
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("target");
    for (auto &target: m_targets) { target.draw(shader, m_dirlight, m_spotlight); }
}

void MainController::awake_targets() { for (auto &target: m_targets) { target.m_active = true; } }

void MainController::update_targets() { for (auto &target: m_targets) { target.update(engine::core::Controller::get<engine::platform::PlatformController>()->dt()); } }

void MainController::create_boundingbox_targets() { for (auto &target: m_targets) { if (target.m_active) { target.calculate_bounding_box(); } } }

void MainController::check_boundingbox_intersects() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::MOUSE_BUTTON_LEFT).state() == engine::platform::Key::State::JustPressed) { for (auto &target: m_targets) { if (target.check_boundingbox_intersect(m_raycast.origin, m_raycast.dir)) { target.m_active = false; } } }
}

void MainController::create_plane() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    float vertices[] = {
            -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 5.0f,
            -5.0f, -0.5f, 5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            5.0f, -0.5f, 5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f,

            5.0f, -0.5f, 5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 0.0f,
            5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 5.0f, 5.0f,
            -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 5.0f
    };

    m_vao_plane = graphics->set_plane(vertices, sizeof(vertices));
}

void MainController::draw_plane() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("plane");
    auto texture = engine::core::Controller::get<engine::resources::ResourcesController>()->texture("ground");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());
    shader->set_mat4("model", glm::mat4(1.0f));
    shader->set_int("texture_diffuse", 0);
    shader->set_vec3("viewPos", graphics->camera()->Position);

    m_dirlight.apply(shader, "dirlight");
    m_spotlight.apply(shader, "spotlight");

    graphics->draw_plane(m_vao_plane, shader, texture);
}

void MainController::draw_tree() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("tree");
    auto tree = engine::core::Controller::get<engine::resources::ResourcesController>()->model("tree");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    m_dirlight.apply(shader, "dirlight");
    m_spotlight.apply(shader, "spotlight");

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(1.0f, -0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    shader->set_mat4("model", model);
    shader->set_float("shininess", 8.0f);
    shader->set_vec3("viewPos", graphics->camera()->Position);

    tree->draw(shader);
}

void MainController::draw_cabin() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("cabin");
    auto cabin = engine::core::Controller::get<engine::resources::ResourcesController>()->model("cabin1");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    m_dirlight.apply(shader, "dirlight");
    m_spotlight.apply(shader, "spotlight");

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, -0.5f, 1.0f));
    model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
    shader->set_mat4("model", model);
    shader->set_float("shininess", 32.0f);
    shader->set_vec3("viewPos", graphics->camera()->Position);

    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model)));
    shader->set_mat3("invNormal", normal_matrix);

    cabin->draw(shader);
}

void MainController::draw_rifle() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("item");
    auto rifle = engine::core::Controller::get<engine::resources::ResourcesController>()->model("ak_47");

    glm::vec3 offset = glm::vec3(0.3f, -0.24f, -0.875f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, offset);
    model = glm::rotate(model, glm::radians(98.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-3.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(3.3f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.7f));

    shader->use();
    shader->set_mat4("model", model);
    shader->set_float("shininess", 32.0f);

    shader->set_mat4("projection", graphics->projection_matrix());

    glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model)));
    shader->set_mat3("invNormal", normal_matrix);

    m_dirlight.apply(shader, "scene_dirlight");
    m_rifle_dirlight.apply(shader, "rifle_dirlight");

    shader->set_vec3("viewPos", graphics->camera()->Position);

    engine::graphics::OpenGL::set_depth_range(0.0, 0.01);
    rifle->draw(shader);
    engine::graphics::OpenGL::set_depth_range(0.0, 1.0);
}

void MainController::draw_skybox() {
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("skybox");
    auto skybox = engine::core::Controller::get<engine::resources::ResourcesController>()->skybox("skybox_night");
    engine::core::Controller::get<engine::graphics::GraphicsController>()->draw_skybox(shader, skybox);
}

void MainController::set_instanced_tree() {
    glm::vec3 offsets[] = {
            glm::vec3(2.8f, -0.5f, 0.0f),
            glm::vec3(3.7f, -0.5f, 0.2f),
            glm::vec3(3.7f, -0.5f, -0.4f),
            glm::vec3(4.1f, -0.5f, -1.0f),
            glm::vec3(4.2f, -0.5f, -0.8f),
            glm::vec3(4.3f, -0.5f, -1.3f),
            glm::vec3(3.3f, -0.5f, 1.1f),
            glm::vec3(3.8f, -0.5f, 1.4f),
            glm::vec3(3.9f, -0.5f, 1.8f),
            glm::vec3(3.8f, -0.5f, 2.2f),
            glm::vec3(4.1f, -0.5f, -1.7f),
            glm::vec3(4.2f, -0.5f, -2.0f),
            glm::vec3(4.0f, -0.5f, -2.3),
            glm::vec3(4.0f, -0.5f, -1.8f),
            glm::vec3(3.7f, -0.5f, -2.9f),
            glm::vec3(3.5f, -0.5f, 3.1f),
            glm::vec3(4.1f, -0.5f, 3.5f),
            glm::vec3(3.8f, -0.5f, 3.9f),
            glm::vec3(3.1f, -0.5f, -3.6f),
            glm::vec3(2.6f, -0.5f, -4.0f),
            glm::vec3(2.1f, -0.5f, -4.1f),
            glm::vec3(1.6f, -0.5f, -3.7f),
            glm::vec3(1.3f, -0.5f, -3.9f),
            glm::vec3(1.2f, -0.5f, -2.9f),
            glm::vec3(0.7f, -0.5f, -4.0f),
            glm::vec3(0.1f, -0.5f, -3.4f),
            glm::vec3(-0.4f, -0.5f, -3.7f),
            glm::vec3(-0.8f, -0.5f, -4.0f),
            glm::vec3(-1.5f, -0.5f, -3.3f),
            glm::vec3(-2.2f, -0.5f, -3.8f),
            glm::vec3(-2.8f, -0.5f, -3.4f),
            glm::vec3(-3.3f, -0.5f, -3.4f),
            glm::vec3(-4.0f, -0.5f, -4.0f),
            glm::vec3(-4.4f, -0.5f, -3.5f),
            glm::vec3(-4.9f, -0.5f, -3.1f)
    };

    m_amount_tree = 34;
    m_model_tree = new glm::mat4[m_amount_tree];
    glm::vec3 scale_factor = glm::vec3(0.5f, 0.5f, 0.5f);

    for (unsigned int i = 0; i < m_amount_tree; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, offsets[i]);
        model = glm::scale(model, scale_factor);
        m_model_tree[i] = model;
    }
}

void MainController::draw_instanced_tree() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("tree");
    auto tree = engine::core::Controller::get<engine::resources::ResourcesController>()->model("tree");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    m_dirlight.apply(shader, "dirlight");
    m_spotlight.apply(shader, "spotlight");

    shader->set_float("shininess", 8.0f);
    shader->set_vec3("viewPos", graphics->camera()->Position);

    graphics->instanced_draw(tree, shader, m_model_tree, m_amount_tree);
}

void MainController::set_crosshair() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    float vertices[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,

            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f
    };

    m_vao_crosshair = graphics->set_crosshair(vertices, sizeof(vertices));
}


void MainController::draw_crosshair() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("crosshair");
    shader->use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.01f));
    shader->set_mat4("model", model);
    shader->set_vec3("color", glm::vec3(0.0f, 1.0f, 0.0f));

    engine::graphics::OpenGL::disable_depth_testing();
    graphics->draw_crosshair(shader, m_vao_crosshair);
    engine::graphics::OpenGL::enable_depth_testing();
}


}// app