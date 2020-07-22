//! \file      ecs_internal.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_ECS_INTERNAL_HPP
#define MANGO_ECS_INTERNAL_HPP

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <mango/scene_component_pool.hpp>
#include <mango/scene_ecs.hpp>
#include <mango/types.hpp>

namespace mango
{
    //! \brief An \a ecsystem for transformation updates.
    class transformation_update_system : public ecsystem_1<transform_component>
    {
      public:
        void update(float, scene_component_pool<transform_component>& transformations) override
        {
            transformations.for_each(
                [&transformations](transform_component& c, int32&) {
                    c.local_transformation_matrix = glm::translate(glm::mat4(1.0), c.position);
                    c.local_transformation_matrix = glm::rotate(c.local_transformation_matrix, c.rotation.x, glm::vec3(c.rotation.y, c.rotation.z, c.rotation.w));
                    c.local_transformation_matrix = glm::scale(c.local_transformation_matrix, c.scale);

                    c.world_transformation_matrix = c.local_transformation_matrix;
                },
                false);
        }
    };

    //! \brief An \a ecsystem for scene graph updates.
    class scene_graph_update_system : public ecsystem_2<node_component, transform_component>
    {
      public:
        void update(float, scene_component_pool<node_component>& nodes, scene_component_pool<transform_component>& transformations) override
        {
            nodes.for_each(
                [&nodes, &transformations](node_component& c, int32& index) {
                    node_component& parent_component = c;
                    entity e                         = nodes.entity_at(index);

                    transform_component* child_transform  = transformations.get_component_for_entity(e);
                    transform_component* parent_transform = transformations.get_component_for_entity(parent_component.parent_entity);
                    if (nullptr != child_transform && nullptr != parent_transform)
                    {
                        child_transform->world_transformation_matrix = parent_transform->world_transformation_matrix * child_transform->local_transformation_matrix;
                    }
                },
                false);
        }
    };

    //! \brief An \a ecsystem for camera updates.
    class camera_update_system : public ecsystem_2<camera_component, transform_component>
    {
      public:
        void update(float, scene_component_pool<camera_component>& cameras, scene_component_pool<transform_component>& transformations) override
        {
            cameras.for_each(
                [&cameras, &transformations](camera_component& c, int32& index) {
                    entity e                       = cameras.entity_at(index);
                    transform_component* transform = transformations.get_component_for_entity(e);
                    if (transform)
                    {
                        glm::vec3 front = glm::normalize(c.target - transform->position);
                        auto right      = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), front)); // TODO Paul: Global up vector?
                        c.up            = glm::normalize(glm::cross(front, right));
                        c.view          = glm::lookAt(transform->position, c.target, c.up);
                        if (c.type == camera_type::perspective_camera)
                        {
                            c.projection = glm::perspective(c.vertical_field_of_view, c.aspect, c.z_near, c.z_far);
                        }
                        else if (c.type == camera_type::orthographic_camera)
                        {
                            const float distance = c.z_far - c.z_near;
                            c.projection         = glm::ortho(-c.aspect * distance, c.aspect * distance, -distance, distance);
                        }
                        c.view_projection = c.projection * c.view;
                    }
                },
                false);
        }
    };
} // namespace mango

#endif // MANGO_ECS_INTERNAL_HPP