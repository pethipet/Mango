//! \file      editor.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include "editor.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace mango;

MANGO_DEFINE_APPLICATION_MAIN(editor)

bool editor::create()
{
    shared_ptr<context> mango_context = get_context().lock();
    MANGO_ASSERT(mango_context, "Context is expired!");

    window_configuration window_config;
    window_config.set_width(1920).set_height(1080).set_title(get_name());
    shared_ptr<window_system> mango_ws = mango_context->get_window_system().lock();
    MANGO_ASSERT(mango_ws, "Window System is expired!");
    mango_ws->configure(window_config);

    render_configuration render_config;
    render_config.set_base_render_pipeline(render_pipeline::deferred_pbr).set_vsync(true).enable_render_step(mango::render_step::ibl);
    shared_ptr<render_system> mango_rs = mango_context->get_render_system().lock();
    MANGO_ASSERT(mango_rs, "Render System is expired!");
    mango_rs->configure(render_config);

    shared_ptr<scene> application_scene = std::make_shared<scene>("test_scene");
    mango_context->register_scene(application_scene);

    // camera
    m_main_camera = application_scene->create_default_camera();

    mango_context->make_scene_current(application_scene);

    shared_ptr<input_system> mango_is = mango_context->get_input_system().lock();
    MANGO_ASSERT(mango_is, "Input System is expired!");
    // At the moment it is required to configure the window before setting any input related stuff.
    // scene and environment drag'n'drop
    mango_is->set_drag_and_drop_callback([this](int count, const char** paths) {
        shared_ptr<context> mango_context = get_context().lock();
        MANGO_ASSERT(mango_context, "Context is expired!");
        auto application_scene = mango_context->get_current_scene();
        for (uint32 i = 0; i < count; i++)
        {
            string path = string(paths[i]);
            auto ext    = path.substr(path.find_last_of(".") + 1);
            if (ext == "hdr")
            {
                application_scene->remove_entity(m_environment);
                m_environment = application_scene->create_environment_from_hdr(path, 0.125f);
            }
            else if (ext == "glb" || ext == "gltf")
            {
                for (entity e : m_model)
                    application_scene->remove_entity(e);

                m_model = application_scene->create_entities_from_model(path);
            }
        }
    });

    // temporary editor camera controls
    m_camera_rotation     = glm::vec2(0.0f, glm::radians(90.0f));
    m_last_mouse_position = glm::vec2(0.0f);
    mango_is->set_mouse_position_callback([this](float x_position, float y_position) {
        shared_ptr<context> mango_context = get_context().lock();
        MANGO_ASSERT(mango_context, "Context is expired!");

        shared_ptr<input_system> mango_is = mango_context->get_input_system().lock();
        MANGO_ASSERT(mango_is, "Input System is expired!");

        if (mango_is->get_mouse_button(mouse_button::MOUSE_BUTTON_LEFT) == input_action::RELEASE || glm::length(m_last_mouse_position) == 0.0f)
        {
            m_last_mouse_position = glm::vec2(x_position, y_position);
            return;
        }

        glm::vec2 diff = glm::vec2(x_position, y_position) - m_last_mouse_position;
        diff.y *= -1.0f;

        m_camera_rotation += diff * 0.005f;
        m_camera_rotation.y = glm::clamp(m_camera_rotation.y, glm::radians(15.0f), glm::radians(165.0f));
        m_camera_rotation.x = m_camera_rotation.x < 0.0f ? m_camera_rotation.x + glm::radians(360.0f) : m_camera_rotation.x;
        m_camera_rotation.x = glm::mod(m_camera_rotation.x, glm::radians(360.0f));

        m_last_mouse_position = glm::vec2(x_position, y_position);
    });
    m_camera_radius = 1.0f;
    mango_is->set_mouse_scroll_callback([this](float x_offset, float y_offset) {
        if (y_offset < 0)
        {
            m_camera_radius *= 1.04f;
        }
        else
        {
            m_camera_radius /= 1.04f;
        }
        m_camera_radius = glm::clamp(m_camera_radius, 0.125f, 12.5f);
    });

    return true;
}

void editor::update(float dt)
{
    MANGO_UNUSED(dt);
    shared_ptr<context> mango_context = get_context().lock();

    shared_ptr<input_system> mango_is = mango_context->get_input_system().lock();
    MANGO_ASSERT(mango_is, "Input System is expired!");

    MANGO_ASSERT(mango_context, "Context is expired!");
    auto application_scene = mango_context->get_current_scene();
    auto cam_transform     = application_scene->get_transform_component(m_main_camera);
    auto cam_data          = application_scene->get_camera_component(m_main_camera);

    cam_transform->position.x = cam_data->target.x + m_camera_radius * (sin(m_camera_rotation.y) * cos(m_camera_rotation.x));
    cam_transform->position.y = cam_data->target.y + m_camera_radius * (cos(m_camera_rotation.y));
    cam_transform->position.z = cam_data->target.z + m_camera_radius * (sin(m_camera_rotation.y) * sin(m_camera_rotation.x));
}

void editor::destroy() {}
