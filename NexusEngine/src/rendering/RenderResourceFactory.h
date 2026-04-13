#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Common/interface/BasicMath.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Shader.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace NexusEngine
{
    struct Material;
    struct Mesh;

    // Vertex format containing position, normal, and texture coordinates.
    struct VertexPosNormalUV
    {
        // Object-space position.
        float position[3];

        // Object-space normal.
        float normal[3];

        // Texture coordinates.
        float uv[2];
    };

    // Vertex format containing position, color, and texture coordinates.
    struct VertexPosColorUV
    {
        // Object-space position.
        float position[3];

        // Vertex color.
        float color[4];

        // Texture coordinates.
        float uv[2];
    };

    // Factory for creating shaders, materials, meshes, and cached pipelines.
    class RenderResourceFactory
    {
    public:
        // Cached pipeline objects for a material/mesh/output combination.
        struct CachedPipeline
        {
            // Pipeline state object for the cached variant.
            Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pipelineState;

            // Shared resource binding template for the cached variant.
            Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_shaderResourceBindingTemplate;
        };

        /// <summary>
        /// Creates a resource factory bound to a device and context.
        /// </summary>
        /// <param name="device">Render device used to allocate resources.</param>
        /// <param name="context">Device context used for uploads and setup.</param>
        RenderResourceFactory(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context);

        /// <summary>
        /// Creates a shader from source text.
        /// </summary>
        /// <param name="source">Shader source code.</param>
        /// <param name="entryPoint">Shader entry point name.</param>
        /// <param name="type">Shader stage to create.</param>
        /// <param name="name">Optional shader name.</param>
        /// <returns>The created shader, or null on failure.</returns>
        Diligent::RefCntAutoPtr<Diligent::IShader> CreateShaderFromSource(
            const char* source,
            const char* entryPoint,
            Diligent::SHADER_TYPE type,
            const char* name = nullptr);

        /// <summary>
        /// Creates a shader from a file on disk.
        /// </summary>
        /// <param name="filePath">Path to the shader file.</param>
        /// <param name="entryPoint">Shader entry point name.</param>
        /// <param name="type">Shader stage to create.</param>
        /// <param name="name">Optional shader name.</param>
        /// <returns>The created shader, or null on failure.</returns>
        Diligent::RefCntAutoPtr<Diligent::IShader> CreateShaderFromFile(
            const char* filePath,
            const char* entryPoint,
            Diligent::SHADER_TYPE type,
            const char* name = nullptr);

        /// <summary>
        /// Creates a graphics pipeline state object.
        /// </summary>
        /// <param name="name">Pipeline name.</param>
        /// <param name="vertexShader">Vertex shader to bind.</param>
        /// <param name="pixelShader">Pixel shader to bind.</param>
        /// <param name="inputLayout">Vertex input layout.</param>
        /// <param name="topology">Primitive topology.</param>
        /// <param name="cullMode">Face culling mode.</param>
        /// <param name="depthTestEnabled">Whether depth testing is enabled.</param>
        /// <param name="depthWriteEnabled">Whether depth writes are enabled.</param>
        /// <param name="rtvFormat">Render target format.</param>
        /// <param name="dsvFormat">Depth target format.</param>
        /// <returns>The created pipeline state, or null on failure.</returns>
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> CreateGraphicsPipeline(
            const char* name,
            Diligent::IShader* vertexShader,
            Diligent::IShader* pixelShader,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            Diligent::CULL_MODE cullMode = Diligent::CULL_MODE_BACK,
            bool depthTestEnabled = true,
            bool depthWriteEnabled = true,
            Diligent::TEXTURE_FORMAT rtvFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB,
            Diligent::TEXTURE_FORMAT dsvFormat = Diligent::TEX_FORMAT_D32_FLOAT);

        /// <summary>
        /// Creates a material from in-memory shader sources.
        /// </summary>
        /// <param name="name">Material name.</param>
        /// <param name="vsSource">Vertex shader source.</param>
        /// <param name="psSource">Pixel shader source.</param>
        /// <param name="inputLayout">Vertex input layout.</param>
        /// <param name="topology">Primitive topology.</param>
        /// <returns>The created material, or null on failure.</returns>
        Material* CreateMaterial(
            const char* name,
            const char* vsSource,
            const char* psSource,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        /// <summary>
        /// Creates a material from shader files.
        /// </summary>
        /// <param name="name">Material name.</param>
        /// <param name="vsFilePath">Vertex shader file path.</param>
        /// <param name="psFilePath">Pixel shader file path.</param>
        /// <param name="inputLayout">Vertex input layout.</param>
        /// <param name="topology">Primitive topology.</param>
        /// <returns>The created material, or null on failure.</returns>
        Material* CreateMaterialFromFiles(
            const char* name,
            const char* vsFilePath,
            const char* psFilePath,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        /// <summary>
        /// Creates the default material used by the engine.
        /// </summary>
        /// <returns>The created material, or null on failure.</returns>
        Material* CreateDefaultMaterial();

        /// <summary>
        /// Creates the unlit material used by the sample.
        /// </summary>
        /// <returns>The created material, or null on failure.</returns>
        Material* CreateUnlitMaterial();

        /// <summary>
        /// Compiles any shader resources that need to be prepared eagerly.
        /// </summary>
        /// <returns>True if compilation succeeds; otherwise false.</returns>
        bool CompileAllShaders();

        /// <summary>
        /// Returns a cached pipeline variant or creates one if needed.
        /// </summary>
        /// <param name="material">Material used to build the pipeline.</param>
        /// <param name="mesh">Mesh used to build the pipeline.</param>
        /// <param name="rtvFormat">Render target format.</param>
        /// <param name="dsvFormat">Depth target format.</param>
        /// <returns>A cached pipeline variant.</returns>
        CachedPipeline* GetOrCreatePipeline(
            Material* material,
            Mesh* mesh,
            Diligent::TEXTURE_FORMAT rtvFormat,
            Diligent::TEXTURE_FORMAT dsvFormat);

        /// <summary>
        /// Creates a mesh from raw vertex and index data.
        /// </summary>
        /// <param name="name">Mesh name.</param>
        /// <param name="vertexData">Pointer to vertex data.</param>
        /// <param name="vertexCount">Number of vertices.</param>
        /// <param name="vertexStride">Vertex size in bytes.</param>
        /// <param name="indexData">Optional pointer to index data.</param>
        /// <param name="indexCount">Number of indices.</param>
        /// <param name="topology">Primitive topology.</param>
        /// <returns>The created mesh, or null on failure.</returns>
        Mesh* CreateMesh(
            const char* name,
            const void* vertexData,
            Diligent::Uint32 vertexCount,
            Diligent::Uint32 vertexStride,
            const Diligent::Uint32* indexData = nullptr,
            Diligent::Uint32 indexCount = 0,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        /// <summary>
        /// Creates a unit cube mesh.
        /// </summary>
        /// <returns>The created mesh, or null on failure.</returns>
        Mesh* CreateCubeMesh();

        /// <summary>
        /// Creates a simple plane mesh.
        /// </summary>
        /// <returns>The created mesh, or null on failure.</returns>
        Mesh* CreatePlaneMesh();

        /// <summary>
        /// Creates a sphere mesh with configurable segment count.
        /// </summary>
        /// <param name="segments">Number of sphere segments.</param>
        /// <returns>The created mesh, or null on failure.</returns>
        Mesh* CreateSphereMesh(int segments = 32);

        /// <summary>
        /// Creates a constant buffer of the requested size.
        /// </summary>
        /// <param name="name">Buffer name.</param>
        /// <param name="size">Buffer size in bytes.</param>
        /// <returns>The created constant buffer, or null on failure.</returns>
        Diligent::RefCntAutoPtr<Diligent::IBuffer> CreateConstantBuffer(
            const char* name,
            Diligent::Uint32 size);

    private:
        struct PipelineKey
        {
            const Material* m_material = nullptr;
            const Mesh* m_mesh = nullptr;
            Diligent::TEXTURE_FORMAT m_rtvFormat = Diligent::TEX_FORMAT_UNKNOWN;
            Diligent::TEXTURE_FORMAT m_dsvFormat = Diligent::TEX_FORMAT_UNKNOWN;

            bool operator==(const PipelineKey& other) const = default;
        };

        struct PipelineKeyHasher
        {
            size_t operator()(const PipelineKey& key) const noexcept;
        };

        Diligent::IRenderDevice* m_device;
        Diligent::IDeviceContext* m_context;
        std::unordered_map<PipelineKey, CachedPipeline, PipelineKeyHasher> m_pipelineCache;
    };
} // namespace NexusEngine
