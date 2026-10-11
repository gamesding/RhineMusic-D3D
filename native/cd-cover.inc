// A reusable, transparent disc image for 2D artwork and the 3D print atlas.
// Optical reflection is evaluated separately in the scene shaders.
static std::shared_ptr<Pixels> compactDisc(IWICImagingFactory* factory,IDWriteFactory* write,IDWriteFontCollection* fonts){
    constexpr UINT size=1024;auto result=std::make_shared<Pixels>();result->width=result->height=size;result->rgba.resize(size*size*4);
    for(UINT y=0;y<size;++y)for(UINT x=0;x<size;++x){float px=(x+.5f)/size-.5f,py=(y+.5f)/size-.5f,r=std::hypot(px,py),a=std::atan2(py,px);auto* pixel=&result->rgba[(size_t(y)*size+x)*4];if(r>.46f||r<.060f)continue;
        float metallic=.75f+.09f*std::cos(a*2-.8f)+.035f*std::cos(a*7)+.002f*std::sin(r*1200);
        if(r<.095f)metallic=.57f+.08f*std::cos(a*2);if(r>.451f)metallic=.40f+.16f*std::cos(a*2);if(std::abs(r-.108f)<.002f||std::abs(r-.443f)<.0015f)metallic=.92f;
        pixel[0]=static_cast<unsigned char>(std::clamp(metallic,0.f,1.f)*255);pixel[1]=static_cast<unsigned char>(std::clamp(metallic+.02f,0.f,1.f)*255);pixel[2]=static_cast<unsigned char>(std::clamp(metallic+.04f,0.f,1.f)*255);pixel[3]=255;
    }
    for(size_t i=0;i<result->rgba.size();i+=4)std::swap(result->rgba[i],result->rgba[i+2]);
    Com<IWICBitmap> bitmap;check(factory->CreateBitmapFromMemory(size,size,GUID_WICPixelFormat32bppPBGRA,size*4,UINT(result->rgba.size()),result->rgba.data(),&bitmap),"Disc bitmap");
    Com<ID2D1Factory> d2d;check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,d2d.GetAddressOf()),"Disc D2D");Com<ID2D1RenderTarget> target;check(d2d->CreateWicBitmapRenderTarget(bitmap.Get(),D2D1::RenderTargetProperties(),&target),"Disc target");
    Com<ID2D1SolidColorBrush> brush;check(target->CreateSolidColorBrush(D2D1::ColorF(.17f,.20f,.23f,.72f),&brush),"Disc lettering");Com<IDWriteTextFormat> format;check(write->CreateTextFormat(L"MiSans",fonts,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,31,L"en-US",&format),"Disc text");format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    target->BeginDraw();target->DrawTextW(L"COMPACT DISC",12,format.Get(),D2D1::RectF(270,705,754,758),brush.Get());Com<IDWriteTextFormat> captionFont;check(write->CreateTextFormat(L"MiSans",fonts,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,16,L"en-US",&captionFont),"Disc subtitle");captionFont->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);target->DrawTextW(L"DIGITAL AUDIO",13,captionFont.Get(),D2D1::RectF(270,760,754,810),brush.Get());check(target->EndDraw(),"Disc label");
    check(bitmap->CopyPixels(nullptr,size*4,UINT(result->rgba.size()),result->rgba.data()),"Disc pixels");for(size_t i=0;i<result->rgba.size();i+=4)std::swap(result->rgba[i],result->rgba[i+2]);return result;
}
