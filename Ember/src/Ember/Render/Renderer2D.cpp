#include "ebpch.h"
#include "Renderer2D.h"

#include "Ember/Core/Core.h"
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

	struct RendererData2D {
		static const unsigned int MaxQuads = 1024;
		static const unsigned int MaxVertices = MaxQuads * 4;
		static const unsigned int MaxIndices = MaxQuads * 6;
		static const unsigned int MaxTextureSlots = 32;

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

		unsigned int QuadIndicesInBatch;

		std::array<SharedPtr<Texture>, MaxTextureSlots> TextureSlots;
		unsigned int TextureSlotIndex = 1;	// 0 is default

		Matrix4f ViewProjectionMatrix = Matrix4f(1.0f);
	};

	static ScopedPtr<RendererData2D> s_RendererData;


	void Renderer2D::Init()
	{
		s_RendererData = ScopedPtr<RendererData2D>::Create();

		s_RendererData->QuadBufferStart = new QuadVertex[s_RendererData->MaxVertices];

		s_RendererData->QuadVertexArray = VertexArray::Create();
		s_RendererData->QuadVertexBuffer = VertexBuffer::Create((unsigned int)(s_RendererData->MaxVertices * sizeof(QuadVertex)));
		s_RendererData->QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" },
			{ ShaderDataType::Float2, "v_TextureCoords"},
			{ ShaderDataType::Float, "v_TextureIndex"}
			});

		// Generic index buffer population
		auto quadIndexBufferData = new unsigned int[s_RendererData->MaxIndices];
		unsigned int vertexOffset = 0;
		for (unsigned int i = 0; i < s_RendererData->MaxIndices; i += 6)
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
		s_RendererData->QuadShader = Shader::Create("Ember/assets/shaders/Renderer2D_Quad.glsl");
		s_RendererData->QuadShader->Bind();
		for (unsigned int i = 0; i < s_RendererData->MaxTextureSlots; i++)
			s_RendererData->QuadShader->SetInt("u_Textures[" + std::to_string(i) + "]", i);

		// Default white texture
		s_RendererData->DefaultTexture = Texture::Create();
		uint32_t whiteTextureData = 0xffffffff;
		s_RendererData->DefaultTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_RendererData->TextureSlots[0] = s_RendererData->DefaultTexture;
	}

	void Renderer2D::Shutdown()
	{
		s_RendererData.Reset();
		delete[] s_RendererData->QuadBufferStart;
	}

	void Renderer2D::BeginFrame(CameraComponent& cameraComponent, const Matrix4f& transform)
	{
		RenderAction::UseBlending(true);

		Matrix4f viewMatrix = Math::Inverse(transform);
		s_RendererData->ViewProjectionMatrix = cameraComponent.Camera.GetProjectionMatrix() * viewMatrix;
		s_RendererData->QuadShader->Bind();
		s_RendererData->QuadShader->SetMatrix4("u_ViewProjection", s_RendererData->ViewProjectionMatrix);

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

	void Renderer2D::FlushBatch()
	{
		if (s_RendererData->QuadIndicesInBatch)
		{
			for (unsigned int i = 0; i < s_RendererData->TextureSlotIndex; i++)
				s_RendererData->TextureSlots[i]->Bind(i);

			auto size = (unsigned int)((char*)s_RendererData->QuadBufferCurrent - (char*)s_RendererData->QuadBufferStart);
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

		for (unsigned int i = 0; i < 4; i++)
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

		for (unsigned int i = 1; i < s_RendererData->TextureSlotIndex; i++)
		{
			if (s_RendererData->TextureSlots[i] == nullptr)
				break;

			if (*texture == s_RendererData->TextureSlots[i])
			{
				texIndex = (float)i;
				break;
			}
		}

		// If no texture was found, need to create new one
		if (texIndex == 0.0f)
		{
			if (s_RendererData->TextureSlotIndex >= s_RendererData->MaxTextureSlots)
				NextBatch();

			texIndex = (float)s_RendererData->TextureSlotIndex;
			s_RendererData->TextureSlots[s_RendererData->TextureSlotIndex++] = texture;
		}

		if (s_RendererData->QuadIndicesInBatch >= s_RendererData->MaxIndices)
			NextBatch();

		for (unsigned int i = 0; i < 4; i++)
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