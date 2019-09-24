///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

class FBO {
public:
    unsigned int fboID;
    unsigned int textureID[8];
    int width, height;  // Size of the texture.
	unsigned int texCount;

	void CreateFBO(const int w, const int h, const int t_count = 1);
    void Bind();
    void Unbind();
	void BindTexture(unsigned int index = 0, unsigned int slot = 0);
};
