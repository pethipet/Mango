//! \file      render_system_impl.cpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#include <rendering/pipelines/deferred_pbr_render_system.hpp>
#include <rendering/render_system_impl.hpp>

using namespace mango;

render_system_impl::render_system_impl(const shared_ptr<context_impl>& context)
    : m_shared_context(context)
{
    m_command_buffer = command_buffer::create();
}

render_system_impl::~render_system_impl() {}

bool render_system_impl::create()
{
    if (m_current_render_system)
        return m_current_render_system->create();

    return true;
}

void render_system_impl::configure(const render_configuration& configuration)
{
    bool success                        = false;
    render_pipeline configured_pipeline = configuration.get_base_render_pipeline();
    if (!m_current_render_system || configured_pipeline != m_current_render_system->get_base_render_pipeline())
    {
        // Pipeline has changed and the current render system needs to be recreated.
        if (m_current_render_system)
            m_current_render_system->destroy();

        switch (configured_pipeline)
        {
        case deferred_pbr:
            m_current_render_system = std::make_shared<deferred_pbr_render_system>(m_shared_context);
            success                 = m_current_render_system->create();
            MANGO_ASSERT(success, "Creation of the deferred pbr render system did fail!");
            m_current_render_system->m_command_buffer = m_command_buffer;
            break;
        default:
            MANGO_LOG_ERROR("Render pipeline is unknown and the render system cannot be created!");
            break;
        }
    }

    if (m_current_render_system)
        m_current_render_system->configure(configuration);

    if (!success)
    {
        MANGO_LOG_ERROR("Render pipeline  failed to create and render system cannot be created!");
    }
}

void render_system_impl::begin_render()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->begin_render();
}

void render_system_impl::finish_render()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->finish_render();
}

void render_system_impl::set_viewport(int32 x, int32 y, int32 width, int32 height)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    MANGO_ASSERT(x >= 0, "Viewport x position has to be positive!");
    MANGO_ASSERT(y >= 0, "Viewport y position has to be positive!");
    MANGO_ASSERT(width >= 0, "Viewport width has to be positive!");
    MANGO_ASSERT(height >= 0, "Viewport height has to be positive!");
    m_current_render_system->set_viewport(x, y, width, height);
}

void render_system_impl::set_model_info(const glm::mat4& model_matrix, bool has_normals, bool has_tangents)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->set_model_info(model_matrix, has_normals, has_tangents);
}

void render_system_impl::draw_mesh(const material_ptr& mat, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    MANGO_ASSERT(first >= 0, "The first index has to be greater than 0!");
    MANGO_ASSERT(count >= 0, "The index count has to be greater than 0!");
    MANGO_ASSERT(instance_count >= 0, "The instance count has to be greater than 0!");
    m_current_render_system->draw_mesh(mat, topology, first, count, type, instance_count);
}

void render_system_impl::set_view_projection_matrix(const glm::mat4& view_projection)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->set_view_projection_matrix(view_projection);
}

void render_system_impl::set_environment_texture(const texture_ptr& hdr_texture, float render_level)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->set_environment_texture(hdr_texture, render_level);
}

framebuffer_ptr render_system_impl::get_backbuffer()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    return m_current_render_system->get_backbuffer();
}

void render_system_impl::update(float dt)
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    m_current_render_system->update(dt);
}

void render_system_impl::destroy()
{
    if (m_current_render_system)
        m_current_render_system->destroy();
}

render_pipeline render_system_impl::get_base_render_pipeline()
{
    MANGO_ASSERT(m_current_render_system, "Current render sytem not valid!");
    return m_current_render_system->get_base_render_pipeline();
}