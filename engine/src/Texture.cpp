#include "Debug.h"
#include "GLLoader.h"
#include "FileUtil.h"
#include "Texture.h"
#include "EnumUtil.h"

namespace fury
{
	Texture::Ptr Texture::Create(const std::string &name)
	{
		return std::make_shared<Texture>(name);
	}

	Texture::Texture(const std::string &name) : Entity(name)
	{
		m_TypeIndex = typeid(Texture);
	}

	Texture::~Texture()
	{
		DeleteBuffer();
	}

	bool Texture::Load(const void* wrapper)
	{
		EnumUtil::Ptr enumUtil = EnumUtil::Instance();

		std::string str;

		if (!IsObject(wrapper)) return false;

		if (!LoadMemberValue(wrapper, "format", str))
		{
			LOGE << "Texture param 'format' not found!";
			return false;
		}
		auto format = enumUtil->TextureFormatFromString(str);
		
		if (!LoadMemberValue(wrapper, "filter", str))
		{
			LOGE << "Texture param 'filter' not found!";
			return false;
		}
		auto filterMode = enumUtil->FilterModeFromString(str);

		if (!LoadMemberValue(wrapper, "wrap", str))
		{
			LOGE << "Texture param 'wrap' not found!";
			return false;
		}
		auto wrapMode = enumUtil->WrapModeFromString(str);

		int width, height;
		if (!LoadMemberValue(wrapper, "width", width) || !LoadMemberValue(wrapper, "height", height))
		{
			LOGE << "Texture param 'width/height' not found!";
			return false;
		}

		bool mipmap = false;
		LoadMemberValue(wrapper, "mipmap", mipmap);

		SetFilterMode(filterMode);
		SetWrapMode(wrapMode);

		CreateEmpty(width, height, format, mipmap);

		return true;
	}

	bool Texture::Save(void* wrapper)
	{
		EnumUtil::Ptr enumUtil = EnumUtil::Instance();

		StartObject(wrapper);

		SaveKey(wrapper, "name");
		SaveValue(wrapper, m_Name);
		SaveKey(wrapper, "format");
		SaveValue(wrapper, enumUtil->TextureFormatToString(m_Format));
		SaveKey(wrapper, "filter");
		SaveValue(wrapper, enumUtil->FilterModeToString(m_FilterMode));
		SaveKey(wrapper, "wrap");
		SaveValue(wrapper, enumUtil->WrapModeToString(m_WrapMode));
		SaveKey(wrapper, "width");
		SaveValue(wrapper, m_Width);
		SaveKey(wrapper, "height");
		SaveValue(wrapper, m_Height);
		SaveKey(wrapper, "mipmap");
		SaveValue(wrapper, m_Mipmap);

		EndObject(wrapper);

		return true;
	}

	void Texture::CreateFromImage(std::string filePath, bool mipMap)
	{
		DeleteBuffer();

		int channels;
		std::vector<unsigned char> pixels;
		if (FileUtil::Instance()->LoadImage(filePath, pixels, m_Width, m_Height, channels))
		{
			unsigned int internalFormat, imageFormat;

			switch (channels)
			{
			case 1:
				m_Format = TextureFormat::R8;
				internalFormat = GL_R8;
				imageFormat = GL_RED;
				break;
			case 2:
				m_Format = TextureFormat::RG8;
				internalFormat = GL_RG8;
				imageFormat = GL_RG;
				break;
			case 3:
				m_Format = TextureFormat::RGB8;
				internalFormat = GL_RGB8;
				imageFormat = GL_RGB;
				break;
			case 4: 
				m_Format = TextureFormat::RGBA8;
				internalFormat = GL_RGBA8;
				imageFormat = GL_RGBA;
				break;
			default:
				m_Format = TextureFormat::UNKNOW;
				LOGW << channels << " channel image not supported!";
				return;
			}

			m_Mipmap = mipMap;
			m_FilePath = filePath;
			m_Dirty = false;

			glGenTextures(1, &m_ID);
			glBindTexture(GL_TEXTURE_2D, m_ID);

			glTexStorage2D(GL_TEXTURE_2D, m_Mipmap ? FURY_MIPMAP_LEVEL : 1, internalFormat, m_Width, m_Height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, imageFormat, GL_UNSIGNED_BYTE, &pixels[0]);
			
			unsigned int filterMode = EnumUtil::Instance()->FilterModeToUint(m_FilterMode);
			unsigned int wrapMode = EnumUtil::Instance()->WrapModeToUint(m_WrapMode);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

			if (m_Mipmap)
				glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, 0);

			LOGD << m_Name << " [" << m_Width << " x " << m_Height << "]";
		}
	}

	void Texture::CreateEmpty(int width, int height, TextureFormat format, bool mipMap)
	{
		DeleteBuffer();

		if (format == TextureFormat::UNKNOW)
			return;
		
		m_Mipmap = mipMap;
		m_Format = format;
		m_Dirty = false;
		m_Width = width;
		m_Height = height;

		unsigned int internalFormat = EnumUtil::Instance()->TextureFormatToUint(format).second;

		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);
		glTexStorage2D(GL_TEXTURE_2D, m_Mipmap ? FURY_MIPMAP_LEVEL : 1, internalFormat, width, height);

		unsigned int filterMode = EnumUtil::Instance()->FilterModeToUint(m_FilterMode);
		unsigned int wrapMode = EnumUtil::Instance()->WrapModeToUint(m_WrapMode);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

		if (m_Mipmap)
			glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);

		LOGD << m_Name << " [" << m_Width << " x " << m_Height << "]";
	}

	void Texture::DeleteBuffer()
	{
		m_Dirty = true;

		if (m_ID != 0)
		{
			glDeleteTextures(1, &m_ID);
			m_ID = 0;
			m_Width = m_Height = 0;
			m_Format = TextureFormat::UNKNOW;
			m_FilePath = "";
		}
	}

	TextureFormat Texture::GetFormat() const
	{
		return m_Format;
	}

	FilterMode Texture::GetFilterMode() const
	{
		return m_FilterMode;
	}

	void Texture::SetFilterMode(FilterMode mode)
	{
		if (m_FilterMode != mode)
		{
			m_FilterMode = mode;
			if (m_ID != 0)
			{
				glBindTexture(GL_TEXTURE_2D, m_ID);

				unsigned int filterMode = EnumUtil::Instance()->FilterModeToUint(m_FilterMode);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);

				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	WrapMode Texture::GetWrapMode() const
	{
		return m_WrapMode;
	}

	void Texture::SetWrapMode(WrapMode mode)
	{
		if (m_WrapMode != mode)
		{
			m_WrapMode = mode;
			if (m_ID != 0)
			{
				glBindTexture(GL_TEXTURE_2D, m_ID);

				unsigned int wrapMode = EnumUtil::Instance()->WrapModeToUint(m_WrapMode);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	bool Texture::GetMipmap() const
	{
		return m_Mipmap;
	}

	int Texture::GetWidth() const
	{
		return m_Width;
	}

	int Texture::GetHeight() const
	{
		return m_Height;
	}

	unsigned int Texture::GetID() const
	{
		return m_ID;
	}

	std::string Texture::GetFilePath() const
	{
		return m_FilePath;
	}
}