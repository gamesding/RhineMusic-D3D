// Observe only this application's editor focus transitions; never log text.
LRESULT CALLBACK Ui::editorProcedure(HWND handle,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto* ui=reinterpret_cast<Ui*>(data);
    if(message==WM_KILLFOCUS){
        auto next=reinterpret_cast<HWND>(wparam);wchar_t type[96]{};if(next)GetClassNameW(next,type,96);
        for(const auto& [key,item]:ui->inputs_)if(item.handle==handle){
            ui->editorFocusEvents_.push_back({{"key",key},{"stage","editor-kill-focus"},{"targetClass",utf8(type)},{"targetOwned",next==ui->window_||IsChild(ui->window_,next)},{"targetNull",next==nullptr},{"hostVisible",IsWindowVisible(ui->window_)!=FALSE},{"foregroundOwned",GetForegroundWindow()==ui->window_||GetAncestor(GetForegroundWindow(),GA_ROOT)==ui->window_}});
            if(ui->editorFocusEvents_.size()>32)ui->editorFocusEvents_.erase(ui->editorFocusEvents_.begin());break;
        }
    }
    if(message==WM_NCDESTROY)RemoveWindowSubclass(handle,editorProcedure,id);
    return DefSubclassProc(handle,message,wparam,lparam);
}
// The native EDIT owns selection, clipboard, IME and the insertion point. Its
// GDI caret cannot survive a flip-model swap chain presenting over it; paint
// the real editor surface and caret into Direct2D instead, retaining Windows'
// blink interval and the actual native insertion position.
void Ui::editor(Input& item,D2D1_RECT_F rect,D2D1_RECT_F clip){
    GUITHREADINFO gui{sizeof(gui)};bool owns=GetGUIThreadInfo(GetCurrentThreadId(),&gui)&&gui.hwndCaret==item.handle;
    DWORD first=0,last=0;SendMessageW(item.handle,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));
    POINT caret{};if(owns)GetCaretPos(&caret);
    if(first!=item.selectionStart||last!=item.selectionEnd||caret.x!=item.caretPosition.x||caret.y!=item.caretPosition.y){item.bitmapDirty=true;item.caretChanged=model_->now;}
    item.selectionStart=first;item.selectionEnd=last;item.caretPosition=caret;
    int firstVisible=int(SendMessageW(item.handle,EM_GETFIRSTVISIBLELINE,0,0));if(firstVisible!=item.firstVisibleLine){item.firstVisibleLine=firstVisible;item.bitmapDirty=true;}
    if(owns&&!item.caretHidden)item.caretHidden=HideCaret(item.handle)!=FALSE;
    const UINT width=UINT(std::max(1L,item.position.right)),height=UINT(std::max(1L,item.position.bottom));
    auto size=item.bitmap?item.bitmap->GetPixelSize():D2D1_SIZE_U{};
    if(size.width!=width||size.height!=height){item.bitmap.Reset();item.bitmapDirty=true;}
    if(item.bitmapDirty){
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=LONG(width);info.bmiHeader.biHeight=-LONG(height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr;HDC memory=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(memory,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        if(!memory||!bitmap){if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);throw std::runtime_error("Native editor surface allocation failed");}
        auto previous=SelectObject(memory,bitmap);RECT bounds{0,0,LONG(width),LONG(height)};FillRect(memory,&bounds,editorBrush_);
        SendMessageW(item.handle,WM_PRINT,reinterpret_cast<WPARAM>(memory),PRF_CLIENT|PRF_ERASEBKGND);
        // Modern single-line EDITs may omit their text from WM_PRINT on an
        // offscreen/flip-model host. Use the actual native font and character
        // positions for that line; Windows still handles all editing and IME.
        if((GetWindowLongPtrW(item.handle,GWL_STYLE)&ES_MULTILINE)==0){
            const int count=GetWindowTextLengthW(item.handle);std::wstring content(size_t(count)+1,L'\0');GetWindowTextW(item.handle,content.data(),count+1);content.resize(count);if(item.password)content.assign(count,L'*');
            bool cue=false;if(count==0){wchar_t hint[256]{};if(SendMessageW(item.handle,EM_GETCUEBANNER,reinterpret_cast<WPARAM>(hint),256)&&hint[0]){content=hint;cue=true;}}
            FillRect(memory,&bounds,editorBrush_);auto oldFont=SelectObject(memory,editorFont_);SetBkMode(memory,TRANSPARENT);SetTextColor(memory,cue||!IsWindowEnabled(item.handle)?RGB(unsigned(muted_.r*255),unsigned(muted_.g*255),unsigned(muted_.b*255)):editorForeground_);
            RECT format{};SendMessageW(item.handle,EM_GETRECT,0,reinterpret_cast<LPARAM>(&format));TEXTMETRICW metrics{};GetTextMetricsW(memory,&metrics);
            int visible=std::clamp(firstVisible,0,std::max(0,count-1));SIZE prefix{};GetTextExtentPoint32W(memory,content.data(),visible,&prefix);LRESULT position=count?SendMessageW(item.handle,EM_POSFROMCHAR,visible,0):-1;
            int originX=(position==-1?format.left:short(LOWORD(position)))-prefix.cx,originY=position==-1?format.top:short(HIWORD(position));
            ExtTextOutW(memory,originX,originY,ETO_CLIPPED,&bounds,content.data(),UINT(content.size()),nullptr);
            if(item.focused&&first!=last){SIZE begin{},end{};GetTextExtentPoint32W(memory,content.data(),int(std::min<DWORD>(first,count)),&begin);GetTextExtentPoint32W(memory,content.data(),int(std::min<DWORD>(last,count)),&end);
                RECT selected{std::max(0,originX+int(begin.cx)),originY,std::min(int(width),originX+int(end.cx)),originY+metrics.tmHeight};if(selected.right>selected.left){FillRect(memory,&selected,GetSysColorBrush(COLOR_HIGHLIGHT));SetTextColor(memory,GetSysColor(COLOR_HIGHLIGHTTEXT));ExtTextOutW(memory,originX,originY,ETO_CLIPPED,&selected,content.data(),UINT(content.size()),nullptr);}}
            SelectObject(memory,oldFont);
        }
        auto rgba=static_cast<unsigned char*>(pixels);for(size_t i=3;i<size_t(width)*height*4;i+=4)rgba[i]=255;
        HRESULT result=S_OK;if(!item.bitmap)result=renderer_->drawing()->CreateBitmap({width,height},pixels,width*4,D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED)),&item.bitmap);else result=item.bitmap->CopyFromMemory(nullptr,pixels,width*4);
        SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);check(result,"Native editor surface upload");item.bitmapDirty=false;
    }
    rect={item.position.left/scale_,item.position.top/scale_,(item.position.left+LONG(width))/scale_,(item.position.top+LONG(height))/scale_};
    auto drawing=renderer_->drawing();drawing->PushAxisAlignedClip(clip,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);drawing->DrawBitmap(item.bitmap.Get(),rect,1,D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    const UINT blink=GetCaretBlinkTime();bool active=!IsWindowVisible(window_)||GetForegroundWindow()==window_||GetAncestor(GetForegroundWindow(),GA_ROOT)==window_;
    item.caretOn=item.focused&&owns&&active&&first==last&&(blink==INFINITE||blink==0||std::fmod(std::max(0.,model_->now-item.caretChanged),double(blink)*.002)<double(blink)*.001);
    float caretWidth=std::max(1.f/scale_,float(std::max(1L,gui.rcCaret.right-gui.rcCaret.left))/scale_),caretHeight=std::max(12.f,float(gui.rcCaret.bottom-gui.rcCaret.top)/scale_);
    item.caretRect={rect.left+caret.x/scale_,rect.top+caret.y/scale_,rect.left+caret.x/scale_+caretWidth,rect.top+caret.y/scale_+caretHeight};
    if(item.caretOn){drawing->PushAxisAlignedClip(rect,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);fill(item.caretRect,ink_);drawing->PopAxisAlignedClip();}drawing->PopAxisAlignedClip();
}
