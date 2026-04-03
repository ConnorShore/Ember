#include "ebpch.h"
#include "Renderer2D.h"

#include "Ember/Core/Core.h"
#include "Ember/Core/Application.h"
#include "Ember/Math/Math.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"
#include "RenderAction.h"

namespace Ember {

	struct QuadVertex
	{
		Vector3f Position;
		Vector4f Color;
		Vector2f TextureCoords;
		float TextureIndex;
	};

	// Per-frame batch state: CPU-side vertex buffer is filled by DrawQuad calls
	// and flushed to GPU in a single draw when EndFrame or NextBatch is called.
	struct RendererData2D {
		static const uint32_t MaxQuads = 1024;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		// Quads
		SharedPtr<VertexArray> QuadVertexArray;
		SharedPtr<VertexBuffer> QuadVertexBuffer;
		SharedPtr<IndexBuffer> QuadIndexBuffer;

		SharedPtr<Shader> QuadShader;
		SharedPtr<Texture> DefaultTexture;

		QuadVertex* QuadBufferStart;
		QuadVertex* QuadBufferCurrent;

		Vector3f QuadVertexPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f},
		};

		uint32_t QuadIndicesInBatch;

		std::array<SharedPtr<Texture>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;	// 0 is default
	};

	static ScopedPtr<RendererData2D> s_RendererData;


	void Renderer2D::Init()
	{
		s_RendererData = ScopedPtr<RendererData2D>::Create();

		s_RendererData->QuadBufferStart = new QuadVertex[s_RendererData->MaxVertices];

		s_RendererData->QuadVertexArray = VertexArray::Create();
		s_RendererData->QuadVertexBuffer = VertexBuffer::Create(static_cast<uint32_t>(s_RendererData->MaxVertices * sizeof(QuadVertex)));
		s_RendererData->QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" },
			{ ShaderDataType::Float2, "v_TextureCoords"},
			{ ShaderDataType::Float, "v_TextureIndex"}
			});

		// Pre-generate index data: every 4 vertices form a quad drawn as 2 triangles (6 indices)
		auto quadIndexBufferData = new uint32_t[s_RendererData->MaxIndices];
		uint32_t vertexOffset = 0;
		for (uint32_t i = 0; i < s_RendererData->MaxIndices; i += 6)
		{
			quadIndexBufferData[i + 0] = vertexOffset + 0;
			quadIndexBufferData[i + 1] = vertexOffset + 1;
			quadIndexBufferData[i + 2] = vertexOffset + 2;

			quadIndexBufferData[i + 3] = vertexOffset + 2;
			quadIndexBufferData[i + 4] = vertexOffset + 3;
			quadIndexBufferData[i + 5] = vertexOffset + 0;

			vertexOffset += 4;
		}

		s_RendererData->QuadIndexBuffer = IndexBuffer::Create({ quadIndexBufferData, s_RendererData->MaxIndices });
		s_RendererData->QuadVertexArray->SetBuffer(s_RendererData->QuadVertexBuffer, s_RendererData->QuadIndexBuffer);
		delete[] quadIndexBufferData;

		// Todo add to shader/texture libraries
		s_RendererData->QuadShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::Standard2dQuadShad);
		s_RendererData->QuadShader->Bind();
		for (uint32_t i = 0; i < s_RendererData->MaxTextureSlots; i++)
			s_RendererData->QuadShader->SetInt("u_Textures[" + std::to_string(i) + "]", i);

		// Default white texture
		s_RendererData->DefaultTexture = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultWhiteTex);
		s_RendererData->TextureSlots[0] = s_RendererData->DefaultTexture;
	}

	void Renderer2D::Shutdown()
	{
		s_RendererData.Reset();
		delete[] s_RendererData->QuadBufferStart;
	}

	void Renderer2D::BeginFrame()
	{
		RenderAction::UseBlending(true);

		StartBatch();
	}

	void Renderer2D::EndFrame()
	{
		FlushBatch();
	}

	void Renderer2D::StartBatch()
	{
		s_RendererData->QuadIndicesInBatch = 0;
		s_RendererData->QuadBufferCurrent = s_RendererData->QuadBufferStart;

		s_RendererData->TextureSlotIndex = 1;
	}

	// Uploads the accumulated vertex data to the GPU and draws all queued quads
	void Renderer2D::FlushBatch()
	{
		if (s_RendererData->QuadIndicesInBatch)
		{
			for (uint32_t i = 0; i < s_RendererData->TextureSlotIndex; i++)
				s_RendererData->TextureSlots[i]->Bind(i);

			auto size = static_cast<uint32_t>((char*)s_RendererData->QuadBufferCurrent - (char*)s_RendererData->QuadBufferStart);
			s_RendererData->QuadVertexBuffer->SetData(s_RendererData->QuadBufferStart, size);

			s_RendererData->QuadShader->Bind();
			RenderAction::DrawIndexed(s_RendererData->QuadVertexArray, s_RendererData->QuadIndicesInBatch);
		}
	}

	void Renderer2D::NextBatch()
	{
		FlushBatch();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture>& texture)
	{
		Matrix4f transform = Math::Translate(Vector3f(position.x, position.y, 0.0f))
			* Math::Scale(Vector3f(size.x, size.y, 1.0f));
		DrawQuad(transform, color, texture);
	}

	void Renderer2D::DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color)
	{
		Matrix4f transform = Math::Translate(Vector3f(position.x, position.y, 0.0f))
			* Math::Scale(Vector3f(size.x, size.y, 1.0f));
		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const Matrix4f& transform, const Vector4f& color)
	{
		static Vector2f texCoords[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		float texIndex = 0.0f;

		if (s_RendererData->QuadIndicesInBatch >= s_RendererData->MaxIndices)
		{
			EB_CORE_TRACE("Indicie overload, calling next batch!");
			NextBatch();
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			s_RendererData->QuadBufferCurrent->Position = transform * s_RendererData->QuadVertexPositions[i];
			s_RendererData->QuadBufferCurrent->Color = color;
			s_RendererData->QuadBufferCurrent->TextureCoords = texCoords[i];
			s_RendererData->QuadBufferCurrent->TextureIndex = texIndex;
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}


	void Renderer2D::DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture>& texture)
	{
		constexpr Vector2f texCoords[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		float texIndex = 0.0f;

		// Search existing slots for this texture to avoid binding duplicates
		for (uint32_t i = 1; i < s_RendererData->TextureSlotIndex; i++)
		{
			if (s_RendererData->TextureSlots[i] == nullptr)
				break;

			if (*texture == s_RendererData->TextureSlots[i])
			{
				texIndex = (float)i;
				break;
			}
		}

		// If no texture was found, assign it to the next available slot (or flush if full)
		if (texIndex == 0.0f)
		{
			if (s_RendererData->TextureSlotIndex >= s_RendererData->MaxTextureSlots)
				NextBatch();

			texIndex = (float)s_RendererData->TextureSlotIndex;
			s_RendererData->TextureSlots[s_RendererData->TextureSlotIndex++] = texture;
		}

		if (s_RendererData->QuadIndicesInBatch >= s_RendererData->MaxIndices)
			NextBatch();

		for (uint32_t i = 0; i < 4; i++)
		{
			s_RendererData->QuadBufferCurrent->Position = transform * s_RendererData->QuadVertexPositions[i];
			s_RendererData->QuadBufferCurrent->Color = color;
			s_RendererData->QuadBufferCurrent->TextureCoords = texCoords[i];
			s_RendererData->QuadBufferCurrent->TextureIndex = texIndex;
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}

}