#include "legocontainer.h"

#include "lego/legoomni/include/legovideomanager.h"
#include "lego/legoomni/include/misc.h"
#include "mxdirectx/mxdirect3d.h"
#include "tgl/d3drm/impl.h"

DECOMP_SIZE_ASSERT(LegoContainerInfo<LegoTexture>, 0x10);
// DECOMP_SIZE_ASSERT(LegoContainer<LegoTexture>, 0x18);
DECOMP_SIZE_ASSERT(LegoTextureContainer, 0x24);

// FUNCTION: LEGO1 0x10099870
LegoTextureContainer::~LegoTextureContainer()
{
}

// FUNCTION: LEGO1 0x100998e0
LegoTextureInfo* LegoTextureContainer::GetCached(LegoTextureInfo* p_textureInfo)
{
	DDSURFACEDESC desc, newDesc;
	DWORD width, height;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);

	if (p_textureInfo->m_surface->GetSurfaceDesc(&desc) == DD_OK) {
		width = desc.dwWidth;
		height = desc.dwHeight;
	}

	for (LegoCachedTextureList::iterator it = m_cached.begin(); it != m_cached.end(); it++) {
		if ((*it).second == FALSE && (*it).first->m_texture->AddRef() != 0 && (*it).first->m_texture->Release() == 1) {
			if (!strcmp((*it).first->m_name, p_textureInfo->m_name)) {
				LPDIRECTDRAWSURFACE surface = (*it).first->m_surface;
				memset(&newDesc, 0, sizeof(newDesc));
				newDesc.dwSize = sizeof(newDesc);

				if (surface->GetSurfaceDesc(&newDesc) == DD_OK) {
					if (newDesc.dwWidth == width && newDesc.dwHeight == height) {
						(*it).second = TRUE;
						(*it).first->m_texture->AddRef();
						return (*it).first;
					}
				}
			}
		}
	}

	LegoTextureInfo* textureInfo = new LegoTextureInfo();

	textureInfo->m_palette = p_textureInfo->m_palette;
	textureInfo->m_palette->AddRef();

	memset(&newDesc, 0, sizeof(newDesc));
	newDesc.dwWidth = desc.dwWidth;
	newDesc.dwHeight = desc.dwHeight;
	newDesc.dwSize = sizeof(newDesc);
	newDesc.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	newDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
	newDesc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
	newDesc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	newDesc.ddpfPixelFormat.dwRGBBitCount = 8;

	if (VideoManager()->GetDirect3D()->DirectDraw()->CreateSurface(&newDesc, &textureInfo->m_surface, NULL) == DD_OK) {
		RECT rect;
		rect.left = 0;
		rect.right = newDesc.dwWidth - 1;
		rect.top = 0;
		rect.bottom = newDesc.dwHeight - 1;

		textureInfo->m_surface->SetPalette(textureInfo->m_palette);

		if (textureInfo->m_surface->BltFast(0, 0, p_textureInfo->m_surface, &rect, DDBLTFAST_WAIT) != DD_OK) {
			textureInfo->m_surface->Release();
			textureInfo->m_palette->Release();
			delete textureInfo;
			return NULL;
		}
		else {
			if (((TglImpl::RendererImpl*) VideoManager()->GetRenderer())
					->CreateTextureFromSurface(textureInfo->m_surface, &textureInfo->m_texture) != D3DRM_OK) {
				textureInfo->m_surface->Release();
				textureInfo->m_palette->Release();
				delete textureInfo;
				return NULL;
			}
			else {
				textureInfo->m_texture->SetAppData((LPD3DRM_APPDATA) textureInfo);
				m_cached.push_back(LegoCachedTexture(textureInfo, TRUE));

				textureInfo->m_texture->AddRef();

				if (textureInfo->m_name != NULL) {
					delete[] textureInfo->m_name;
				}

				textureInfo->m_name = new char[strlen(p_textureInfo->m_name) + 1];
				strcpy(textureInfo->m_name, p_textureInfo->m_name);
				return textureInfo;
			}
		}
	}

	return NULL;
}

// FUNCTION: LEGO1 0x10099cc0
void LegoTextureContainer::EraseCached(LegoTextureInfo* p_textureInfo)
{
	if (p_textureInfo == NULL) {
		return;
	}

#ifdef COMPAT_MODE
	LegoCachedTextureList::iterator it;
	for (it = m_cached.begin(); it != m_cached.end(); it++) {
#else
	for (LegoCachedTextureList::iterator it = m_cached.begin(); it != m_cached.end(); it++) {
#endif
		if ((*it).first == p_textureInfo) {
			(*it).second = FALSE;

			if (p_textureInfo->m_texture->Release() == TRUE) {
				delete p_textureInfo;
				m_cached.erase(it);
			}

			return;
		}
	}
}
