#include "ebpch.h"
#include "Renderer2D.h"

#include "Ember/Core/Core.h"
#include "Ember/Core/Application.h"
#include "Ember/Math/Math.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "Shader.h"
#include "Texture2D.h"
#include "RenderAction.h"

namespace Ember {

	struct QuadVertex
	{
		Vector3f Position;
		Vector4f Color;
		Vector2f TextureCoords;
		float TextureIndex;

		EntityID EntityID; // For editor picking
		float IsBillboard;
		float LockYAxis;
		Vector3f BillboardCenter;
		Vector2f BillboardOffset;
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
		SharedPtr<Texture2D> DefaultTexture;

		QuadVertex* QuadBufferStart;
		QuadVertex* QuadBufferCurrent;

		Vector3f QuadVertexPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f},
		};

		uint32_t QuadIndicesInBatch;

		std::array<SharedPtr<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;	// 0 is default

		Vector3f CameraPosition = Vector3f(0.0f);
		Vector3f CameraRight = Vector3f(1.0f, 0.0f, 0.0f);
		Vector3f CameraUp = Vector3f(0.0f, 1.0f, 0.0f);
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
			{ ShaderDataType::Float, "v_TextureIndex"},
			{ ShaderDataType::Int, "v_EntityID" },
			{ ShaderDataType::Float, "v_IsBillboard" },
			{ ShaderDataType::Float, "v_LockYAxis" },
			{ ShaderDataType::Float3, "v_BillboardCenter" },
			{ ShaderDataType::Float2, "v_BillboardOffset" }
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
		s_RendererData->QuadVertexArray->AddVertexBuffer(s_RendererData->QuadVertexBuffer);
		s_RendererData->QuadVertexArray->SetIndexBuffer(s_RendererData->QuadIndexBuffer);
		delete[] quadIndexBufferData;

		// Todo add to shader/texture libraries
		s_RendererData->QuadShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::Standard2dQuadShad);
		s_RendererData->QuadShader->Bind();
		for (uint32_t i = 0; i < s_RendererData->MaxTextureSlots; i++)
			s_RendererData->QuadShader->SetInt("u_Textures[" + std::to_string(i) + "]", i);

		// Default white texture
		s_RendererData->DefaultTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
		s_RendererData->TextureSlots[0] = s_RendererData->DefaultTexture;
	}

	void Renderer2D::Shutdown()
	{
		delete[] s_RendererData->QuadBufferStart;
		s_RendererData.Reset();
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

	void Renderer2D::SetBillboardCameraData(const Vector3f& cameraPosition, const Vector3f& cameraRight, const Vector3f& cameraUp)
	{
		s_RendererData->CameraPosition = cameraPosition;
		s_RendererData->CameraRight = cameraRight;
		s_RendererData->CameraUp = cameraUp;
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
			s_RendererData->QuadShader->SetFloat3("u_CameraPosition", s_RendererData->CameraPosition);
			s_RendererData->QuadShader->SetFloat3("u_CameraRight", s_RendererData->CameraRight);
			s_RendererData->QuadShader->SetFloat3("u_CameraUp", s_RendererData->CameraUp);
			RenderAction::DrawIndexed(s_RendererData->QuadVertexArray, s_RendererData->QuadIndicesInBatch);
		}
	}

	void Renderer2D::NextBatch()
	{
		FlushBatch();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture2D>& texture)
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
			s_RendererData->QuadBufferCurrent->EntityID = Constants::Entities::InvalidEntityID;
			s_RendererData->QuadBufferCurrent->IsBillboard = 0.0f;
			s_RendererData->QuadBufferCurrent->LockYAxis = 0.0f;
			s_RendererData->QuadBufferCurrent->BillboardCenter = Vector3f(0.0f);
			s_RendererData->QuadBufferCurrent->BillboardOffset = Vector2f(0.0f);
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}


	void Renderer2D::DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture)
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
			s_RendererData->QuadBufferCurrent->EntityID = Constants::Entities::InvalidEntityID;
			s_RendererData->QuadBufferCurrent->IsBillboard = 0.0f;
			s_RendererData->QuadBufferCurrent->LockYAxis = 0.0f;
			s_RendererData->QuadBufferCurrent->BillboardCenter = Vector3f(0.0f);
			s_RendererData->QuadBufferCurrent->BillboardOffset = Vector2f(0.0f);
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}

	void Renderer2D::DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture, const Vector2f* customTexCoords, EntityID entity)
	{
		float texIndex = 0.0f;

		for (uint32_t i = 1; i < s_RendererData->TextureSlotIndex; i++)
		{
			if (s_RendererData->TextureSlots[i] == nullptr) break;
			if (*texture == s_RendererData->TextureSlots[i])
			{
				texIndex = (float)i;
				break;
			}
		}

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
			s_RendererData->QuadBufferCurrent->TextureCoords = customTexCoords[i];
			s_RendererData->QuadBufferCurrent->TextureIndex = texIndex;
			s_RendererData->QuadBufferCurrent->EntityID = (int)entity;
			s_RendererData->QuadBufferCurrent->IsBillboard = 0.0f;
			s_RendererData->QuadBufferCurrent->LockYAxis = 0.0f;
			s_RendererData->QuadBufferCurrent->BillboardCenter = Vector3f(0.0f);
			s_RendererData->QuadBufferCurrent->BillboardOffset = Vector2f(0.0f);
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}

	void Renderer2D::DrawBillboardQuad(const Vector3f& center, const Vector2f& size, const Vector4f& color, bool lockYAxis /*= false*/)
	{
		DrawBillboardQuad(center, size, color, s_RendererData->DefaultTexture, lockYAxis);
	}

	void Renderer2D::DrawBillboardQuad(const Vector3f& center, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture2D>& texture, bool lockYAxis /*= false*/)
	{
		constexpr Vector2f texCoords[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		float texIndex = 0.0f;

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
			s_RendererData->QuadBufferCurrent->Position = center;
			s_RendererData->QuadBufferCurrent->Color = color;
			s_RendererData->QuadBufferCurrent->TextureCoords = texCoords[i];
			s_RendererData->QuadBufferCurrent->TextureIndex = texIndex;
			s_RendererData->QuadBufferCurrent->EntityID = Constants::Entities::InvalidEntityID;
			s_RendererData->QuadBufferCurrent->IsBillboard = 1.0f;
			s_RendererData->QuadBufferCurrent->LockYAxis = lockYAxis ? 1.0f : 0.0f;
			s_RendererData->QuadBufferCurrent->BillboardCenter = center;
			s_RendererData->QuadBufferCurrent->BillboardOffset = {
				s_RendererData->QuadVertexPositions[i].x * size.x,
				s_RendererData->QuadVertexPositions[i].y * size.y
			};
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}

	void Renderer2D::DrawNineSliceQuad(const Matrix4f& transform, const Vector4f& color,
		const SharedPtr<Texture2D>& texture, const Vector4f& border, EntityID entity)
	{
		if (!texture)
			return;

		float textureWidth = (float)texture->GetWidth();
		float textureHeight = (float)texture->GetHeight();
		if (textureWidth <= 0.0f || textureHeight <= 0.0f)
			return;

		// The transform maps a unit quad, so its basis lengths are the destination size in pixels.
		Vector2f rightAxis(transform[0][0], transform[0][1]);
		Vector2f upAxis(transform[1][0], transform[1][1]);
		float destWidth = Math::Length(rightAxis);
		float destHeight = Math::Length(upAxis);
		if (destWidth <= 0.0f || destHeight <= 0.0f)
			return;

		float left = std::max(0.0f, border.x);
		float bottom = std::max(0.0f, border.y);
		float right = std::max(0.0f, border.z);
		float top = std::max(0.0f, border.w);

		// Shrink opposing borders together when the element is smaller than they are, so the
		// corners meet instead of overlapping and inverting the middle slice.
		if (left + right > destWidth && left + right > 0.0f)
		{
			float scale = destWidth / (left + right);
			left *= scale;
			right *= scale;
		}
		if (bottom + top > destHeight && bottom + top > 0.0f)
		{
			float scale = destHeight / (bottom + top);
			bottom *= scale;
			top *= scale;
		}

		// Slice edges in destination pixels and in UV, bottom-left origin in both.
		const float destX[4] = { 0.0f, left, destWidth - right, destWidth };
		const float destY[4] = { 0.0f, bottom, destHeight - top, destHeight };
		const float texU[4] = { 0.0f, border.x / textureWidth, 1.0f - border.z / textureWidth, 1.0f };
		const float texV[4] = { 0.0f, border.y / textureHeight, 1.0f - border.w / textureHeight, 1.0f };

		for (int row = 0; row < 3; row++)
		{
			for (int column = 0; column < 3; column++)
			{
				float sliceWidth = destX[column + 1] - destX[column];
				float sliceHeight = destY[row + 1] - destY[row];
				if (sliceWidth <= 0.0f || sliceHeight <= 0.0f)
					continue;

				// Position the slice inside the parent's unit quad, then let the parent transform
				// place, rotate and size the whole thing.
				Vector2f sliceCentre(
					(destX[column] + sliceWidth * 0.5f) / destWidth - 0.5f,
					(destY[row] + sliceHeight * 0.5f) / destHeight - 0.5f);

				Matrix4f sliceTransform = transform
					* Math::Translate(Vector3f(sliceCentre, 0.0f))
					* Math::Scale(Vector3f(sliceWidth / destWidth, sliceHeight / destHeight, 1.0f));

				Vector2f sliceTexCoords[4] = {
					{ texU[column],     texV[row] },
					{ texU[column + 1], texV[row] },
					{ texU[column + 1], texV[row + 1] },
					{ texU[column],     texV[row + 1] }
				};

				DrawQuad(sliceTransform, color, texture, sliceTexCoords, entity);
			}
		}
	}

	bool Renderer2D::MeasureString(const std::string& text, const SharedPtr<Font>& font, Vector2f& outMin, Vector2f& outMax)
	{
		if (!font || !font->GetAtlasTexture() || text.empty())
			return false;

		auto atlasTexture = font->GetAtlasTexture();
		const stbtt_bakedchar* glyphData = font->GetGlyphData();

		float cursorX = 0.0f;
		float cursorY = 0.0f;
		bool anyGlyph = false;

		for (char c : text)
		{
			if (c < Font::FirstChar || c >= Font::FirstChar + Font::CharCount)
				continue;

			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(glyphData, atlasTexture->GetWidth(), atlasTexture->GetHeight(),
				c - Font::FirstChar, &cursorX, &cursorY, &q, 1);

			// stbtt is Y-down; DrawString negates Y, so mirror that here to stay in the same space.
			Vector2f quadMin(q.x0, -q.y1);
			Vector2f quadMax(q.x1, -q.y0);

			if (!anyGlyph)
			{
				outMin = quadMin;
				outMax = quadMax;
				anyGlyph = true;
			}
			else
			{
				outMin = Vector2f(std::min(outMin.x, quadMin.x), std::min(outMin.y, quadMin.y));
				outMax = Vector2f(std::max(outMax.x, quadMax.x), std::max(outMax.y, quadMax.y));
			}
		}

		return anyGlyph;
	}

	void Renderer2D::DrawString(const std::string& text, const Matrix4f& transform, const Vector4f& color, const SharedPtr<Font>& font, EntityID entity, bool isScreenSpace /* = false */)
	{
		if (!font || !font->GetAtlasTexture())
			return;

		auto atlasTexture = font->GetAtlasTexture();
		const stbtt_bakedchar* glyphData = font->GetGlyphData();

		// The cursor keeps track of where we are on the line
		float cursorX = 0.0f;
		float cursorY = 0.0f;

		// stb_truetype generates quads in pixel units, so we need to convert to our normalized unit space
		// if in 3D, but if in screen space we can just use pixel units directly (assuming an orthographic projection that matches the screen dimensions)
		const float pixelsPerUnit = 32.0f;
		const float scaleFactor = isScreenSpace ? 1.0f : (1.0f / 32.0f);

		for (char c : text)
		{
			// Skip characters outside our baked ASCII range (like newlines for now)
			if (c < Font::FirstChar || c >= Font::FirstChar + Font::CharCount)
				continue;

			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(glyphData, atlasTexture->GetWidth(), atlasTexture->GetHeight(),
				c - Font::FirstChar, &cursorX, &cursorY, &q, 1);

			// Calculate the size of this specific letter
			float quadWidth = q.x1 - q.x0;
			float quadHeight = q.y1 - q.y0;

			// Center the local quad so it aligns with our Engine's [-0.5, 0.5] quad vertices
			float localX = q.x0 + (quadWidth / 2.0f);

			// stbtt generates Y-down coordinates, so we invert Y for OpenGL
			float localY = -(q.y0 + (quadHeight / 2.0f));

			// Multiply sizes and positions by the scale factor
			localX *= scaleFactor;
			localY *= scaleFactor;
			quadWidth *= scaleFactor;
			quadHeight *= scaleFactor;

			// Map the specific bounding box of the letter in the Atlas to our Quad's vertices
			Vector2f texCoords[4] = {
				{ q.s0, q.t1 }, // Bottom-Left
				{ q.s1, q.t1 }, // Bottom-Right
				{ q.s1, q.t0 }, // Top-Right
				{ q.s0, q.t0 }  // Top-Left
			};

			Matrix4f letterTransform = Math::Translate(Vector3f(localX, localY, 0.0f)) * Math::Scale(Vector3f(quadWidth, quadHeight, 1.0f));
			DrawQuad(transform * letterTransform, color, atlasTexture, texCoords, entity);
		}
	}

}