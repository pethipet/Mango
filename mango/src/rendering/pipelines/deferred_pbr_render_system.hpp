//! \file      deferred_pbr_render_system.hpp
//! \author    Paul Himmler
//! \version   1.0
//! \date      2020
//! \copyright Apache License 2.0

#ifndef MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
#define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP

#include <graphics/framebuffer.hpp>
#include <graphics/gpu_buffer.hpp>
#include <rendering/render_system_impl.hpp>
#include <rendering/steps/cubemap_step.hpp>
#include <rendering/steps/fxaa_step.hpp>
#include <rendering/steps/pipeline_step.hpp>
#include <rendering/steps/shadow_map_step.hpp>

namespace mango
{
    struct camera_data;
    struct environment_data;

    //! \brief A \a render_system using a deferred base pipeline supporting physically based rendering.
    //! \details This system supports physically based materials with and without textures.
    class deferred_pbr_render_system : public render_system_impl
    {
      public:
        //! \brief Constructs the \a deferred_pbr_render_system.
        //! \param[in] context The internally shared context of mango.
        deferred_pbr_render_system(const shared_ptr<context_impl>& context);
        ~deferred_pbr_render_system();

        virtual bool create() override;
        virtual void configure(const render_configuration& configuration) override;
        virtual void setup_cubemap_step(const cubemap_step_configuration& configuration) override;
        virtual void setup_shadow_map_step(const shadow_step_configuration& configuration) override;
        virtual void setup_fxaa_step(const fxaa_step_configuration& configuration) override;
        virtual void begin_render() override;
        virtual void finish_render(float dt) override;
        virtual void set_viewport(int32 x, int32 y, int32 width, int32 height) override;
        virtual void update(float dt) override;
        virtual void destroy() override;
        virtual render_pipeline get_base_render_pipeline() override;

        void begin_mesh(const glm::mat4& model_matrix, bool has_normals, bool has_tangents) override;
        void end_mesh() override;
        void use_material(const material_ptr& mat) override;
        void draw_mesh(const vertex_array_ptr& vertex_array, primitive_topology topology, int32 first, int32 count, index_type type, int32 instance_count) override;
        void submit_light(light_id id, mango_light* light) override;
        void on_ui_widget() override;

        framebuffer_ptr get_backbuffer() override
        {
            return m_backbuffer;
        }

      private:
        //! \brief The gbuffer of the deferred pipeline.
        framebuffer_ptr m_gbuffer;
        //! \brief The backbuffer of the deferred pipeline.
        framebuffer_ptr m_backbuffer;
        //! \brief A \a framebuffer for postprocessing steps of the deferred pipeline.
        framebuffer_ptr m_post_buffer;
        //! \brief The hdr buffer of the deferred pipeline. Used for auto exposure.
        framebuffer_ptr m_hdr_buffer;

        //! \brief The \a command_buffer storing commands to be executed first.
        command_buffer_ptr<min_key> m_begin_render_commands;
        //! \brief The \a command_buffer storing commands regarding globally bound buffers.
        command_buffer_ptr<min_key> m_global_binding_commands;
        //! \brief The \a command_buffer storing commands regarding rendering to the gbuffer.
        command_buffer_ptr<max_key> m_gbuffer_commands;
        //! \brief The \a command_buffer storing commands to render transparent objects.
        command_buffer_ptr<max_key> m_transparent_commands;
        //! \brief The \a command_buffer storing commands to issue lighting calculations for gbuffer objects.
        command_buffer_ptr<min_key> m_lighting_pass_commands;
        //! \brief The \a command_buffer storing commands regarding automatic exposure calculations.
        command_buffer_ptr<min_key> m_exposure_commands;
        //! \brief The \a command_buffer storing commands to composite everything.
        command_buffer_ptr<min_key> m_composite_commands;
        //! \brief The \a command_buffer storing commands to be executed last.
        command_buffer_ptr<min_key> m_finish_render_commands;

        //! \brief The \a shader_program for the deferred geometry pass.
        //! \details This fills the g-buffer for later use in the lighting pass.
        shader_program_ptr m_scene_geometry_pass;

        //! \brief The \a shader_program for the transparency pass.
        //! \details This is a seperate forward pass.
        shader_program_ptr m_transparent_pass;

        //! \brief The \a shader_program for the lighting pass.
        //! \details Utilizes the g-buffer filled before. Outputs hdr.
        shader_program_ptr m_lighting_pass;

        //! \brief The \a shader_program for the luminance buffer construction.
        //! \details Constructs the 'luminance' histogram.
        shader_program_ptr m_construct_luminance_buffer;

        //! \brief The \a shader_program for the luminance buffer reduction.
        //! \details Reduces the 'luminance' histogram to the average luminance.
        shader_program_ptr m_reduce_luminance_buffer;

        //! \brief The shader storage buffer mapping for the luminance histogram
        buffer_ptr m_luminance_histogram_buffer;

        //! \brief The mapped luminance data from the histogram calculation.
        luminance_data* m_luminance_data_mapping;

        //! \brief The \a shader_program for the composing pass.
        //! \details Takes the output in the hdr_buffer and does the final composing to get it to the screen.
        shader_program_ptr m_composing_pass;

        //! \brief The uniform buffer mapping the gpu buffer to the scene uniforms.
        gpu_buffer_ptr m_frame_uniform_buffer;

        //! \brief Uniform buffer struct for renderer data.
        struct renderer_data
        {
            std140_mat4 view_matrix;            //!< The view matrix.
            std140_mat4 projection_matrix;      //!< The projection matrix.
            std140_mat4 view_projection_matrix; //!< The view projection matrix.
            std140_float camera_exposure;       //!< The exposure value of the camera.
            std140_bool shadow_step_enabled;    //!< True, if the shadow map step is enabled and shdows can be calculated.
            std140_float padding1;              //!< Padding.
            std140_float padding2;              //!< Padding.
        } m_renderer_data;                      //!< Current renderer_data.

        //! \brief Uniform buffer struct for model data.
        struct model_data
        {
            std140_mat4 model_matrix;  //!< The model matrix.
            std140_mat3 normal_matrix; //!< The normal matrix.

            std140_bool has_normals;  //!< Specifies if the next mesh has normals as a vertex attribute.
            std140_bool has_tangents; //!< Specifies if the next mesh has tangents as a vertex attribute.

            std140_float padding0; //!< Padding needed for st140 layout.
            std140_float padding1; //!< Padding needed for st140 layout.
        };

        //! \brief Uniform buffer struct for material data.
        struct material_data
        {
            std140_vec4 base_color;     //!< The base color (rgba). Also used as reflection color for metallic surfaces.
            std140_vec3 emissive_color; //!< The emissive color of the material if existent, else (0, 0, 0).
            std140_float metallic;      //!< The metallic value of the material.
            std140_float roughness;     //!< The roughness of the material.

            std140_bool base_color_texture;         //!< Specifies, if the the base color texture is enabled.
            std140_bool roughness_metallic_texture; //!< Specifies, if the component texture is enabled for the metallic value and the roughness value.
            std140_bool occlusion_texture;          //!< Specifies, if the component texture is enabled for the occlusion value.
            std140_bool packed_occlusion;           //!< Specifies, if the occlusion value is packed into the r channel of the roughness_metallic_texture.
            std140_bool normal_texture;             //!< Specifies, if the normal texture is enabled.
            std140_bool emissive_color_texture;     //!< Specifies, if the the emissive color texture is enabled.

            std140_int alpha_mode;     //!< Specifies the alpha mode to render the material with.
            std140_float alpha_cutoff; //!< Specifies the alpha cutoff value to render the material with.

            std140_float padding0; //!< Padding needed for st140 layout.
            std140_float padding1; //!< Padding needed for st140 layout.
        };

        //! \brief Uniform buffer structure for the lighting pass of the deferred pipeline.
        struct lighting_pass_data
        {
            std140_mat4 inverse_view_projection; //!< Inverse camera view projection matrix.
            std140_mat4 view;                    //!< Camera view matrix.
            std140_vec3 camera_position;         //!< Camera position.
            std140_vec4 camera_params;           //!< Camera near and far plane depth value. (zw) unused atm.

            std140_bool debug_view_enabled; //!< True, if any debug view is enabled.

            struct
            {
                std140_bool debug[9]; //!< All debug views.
                // position
                // normal
                // depth
                // base_color
                // reflection_color
                // emission
                // occlusion
                // roughness
                // metallic
            } debug_views; //!< The debug views.

            struct
            {
                std140_bool show_cascades;    //!< Show the shadow cascades.
                std140_bool draw_shadow_maps; //!< Draw the cascade shadow maps.
            } debug_options;                  //!< The debug options.
        } m_lighting_pass_data;               //!< Current lighting_pass_data.

        //! \brief Structure used to cache the \a commands regarding the rendering of the current model/mesh.
        struct model_cache
        {
            int64 model_data_offset;                //!< Caches the offset of the model_data.
            int64 material_data_offset;             //!< Caches the offset of the material_data.
            int8 material_id;                       //!< Caches the material_id.
            glm::vec3 position;                     //!< Caches the transform position (used for example for transparency sorting).
            g_uint base_color_texture_name;         //!< Caches the name of the materials base color texture, or the default one if not existent.
            g_uint roughness_metallic_texture_name; //!< Caches the name of the materials roughness metallic texture, or the default one if not existent.
            g_uint occlusion_texture_name;          //!< Caches the name of the materials occlusion texture, or the default one if not existent.
            g_uint normal_texture_name;             //!< Caches the name of the materials normal texture, or the default one if not existent.
            g_uint emissive_color_texture_name;     //!< Caches the name of the materials emissive color texture, or the default one if not existent.
            bool blend;                             //!< Caches if material needs blending.
            bool face_culling;                      //!< Caches if faces have to be culled for rendering that material.

            //! \brief Returns the validation state of the \a model_cache.
            //! \return True if \a model_cache is valid, else False.
            inline bool valid()
            {
                return model_data_offset >= 0 && material_data_offset >= 0;
            }

            //! \brief Creates an id from the cache and the given \a material_data.
            //! \details The resulting id can be used to sort \a commands by material. This is highly WIP.
            //! \return An int8 to be interpreted as a material id.
            inline int8 create_material_id(material_data& mat_data)
            {
                // should create an id between 0 and 256 that should be as unique as possible.
                // At the moment it depends 'just' on metallic and face culling properties.
                // TODO Paul: ... Make it real.
                int8 v = int8(float(mat_data.metallic) * 10.0f); // 0 - 10
                v += face_culling ? 0 : 100;
                return v;
            }

        } m_active_model; //!< Active model_cache.

        //! \brief Optional additional steps of the deferred pipeline.
        shared_ptr<pipeline_step> m_pipeline_steps[mango::render_step::number_of_step_types];

        //! \brief True if the renderer should draw wireframe, else false.
        bool m_wireframe = false;

        bool create_renderer_resources() override;

        //! \brief Binds the uniform buffer of the renderer.
        //! \param[in,out] camera The \a camera_data of the current camera.
        //! \param[in] camera_exposure The current exposure value.
        void bind_renderer_data_buffer(camera_data& camera, float camera_exposure);

        //! \brief Binds the uniform buffer of the lighting pass.
        //! \param[in,out] camera The \a camera_data of the current camera.
        void bind_lighting_pass_buffer(camera_data& camera);

        //! \brief Calculates exposure and adapts physical camera parameters.
        //! \param[in,out] camera The \a camera_data of the current camera.
        //! \return Returns the calculated camera exposure.
        float apply_exposure(camera_data& camera);

        //! \brief Clears all relevant \a framebuffers. Done in begin_render().
        void clear_framebuffers();
        //! \brief GBuffer pass setup done in begin_render().
        void setup_gbuffer_pass();
        //! \brief Lighting pass setup done in begin_render().
        void setup_lighting_pass();
        //! \brief Transparent pass setup done in begin_render().
        void setup_transparent_pass();

        //! \brief Lighting pass finalization done in finish_render().
        //! \param[in] step_shadow_map The shared pointer to the \a shadow_map_step, or null.
        void finalize_lighting_pass(const std::shared_ptr<shadow_map_step>& step_shadow_map);
        //! \brief Auto exposure compute passes done in finish_render().
        //! \param[in] dt Past time since last call.
        void calculate_auto_exposure(float dt);
        //! \brief Composite pass done in finish_render().
        //! \details Renders to backbuffer, when this is the last pass, else to the post buffer.
        //! \param[in] render_to_pp True if there are other passes after composite, else False.
        void composite_pass(bool render_to_pp);
        //! \brief Frame finalization pass with setup for next frame done in finish_render().
        void end_frame_and_sync();
        //! \brief Sorts and executes all \a command_buffers in the correct order.
        //! \param[in] ibl_command_buffer The shared pointer to the \a command_buffer of the \a cubemap_step, or null.
        //! \param[in] shadow_command_buffer The shared pointer to the \a command_buffer of the \a shadow_map_step, or null.
        //! \param[in] fxaa_command_buffer The shared pointer to the \a command_buffer of the \a fxaa_step, or null.
        void execute_commands(const command_buffer_ptr<min_key>& ibl_command_buffer, const command_buffer_ptr<max_key>& shadow_command_buffer, const command_buffer_ptr<min_key>& fxaa_command_buffer);

        //! \brief Sets up commands for a new mesh.
        //! \param[in,out] draw_buffer The command_buffer to add the commands to.
        //! \param[in] mesh_key The key used for sorting later on.
        //! \param[in] simplified True if mesh should bound for shadow mapping, else false.
        //! \return The last \a bind_texture_command to append to.
        bind_texture_command* begin_mesh_draw(const command_buffer_ptr<max_key>& draw_buffer, max_key mesh_key, bool simplified = false);
        //! \brief Sets up commands for a material.
        //! \param[in,out] draw_buffer The command_buffer to add the commands to.
        //! \param[in] last_command The previous command to append to.
        //! \return The last \a bind_texture_command to append to.
        bind_texture_command* bind_material_textures(const command_buffer_ptr<max_key>& draw_buffer, bind_buffer_command* last_command);

#ifdef MANGO_DEBUG
        void cleanup_texture_bindings(const command_buffer_ptr<max_key>& draw_buffer, bind_vertex_array_command* last_command);
#endif // MANGO_DEBUG
    };

} // namespace mango

#endif // #define MANGO_DEFERRED_PBR_RENDER_SYSTEM_HPP
