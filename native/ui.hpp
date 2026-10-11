#pragma once
#include "renderer.hpp"
#include "controller.hpp"
#include "title.hpp"
#include "ruler.hpp"
#include <unordered_map>
namespace rhine {
struct UiModel {
    json playingAlbum=json::object();
    json preferences=json::object(),library=json::object(),player=json::object(),nativeStatus=json::object(),sonyStatus=json::object(),ftp=json::object(),config=json::object(),onlineSources=json::object(),onlineResults=json::object(),onlineLibrary=json::object(),genres=json::object();
    bool onlineBusy=false;std::string onlineQuery,onlineError,subsonicStatus;unsigned libraryRevision=0;
    int selected=0;bool detail=false,booting=true,ready=false;std::string shelf="local",panel,message;int rowNavigation=0;unsigned navigationSerial=0;
    double messageTime=0,now=0;float night=0,detailProgress=0,detailOpacity=0,browseOpacity=0,detailOffset=0;bool detailSurface=false;json graphics=json::object();
    bool viewer=false;json viewerState=json::object();
    json cdAngles=json::object();bool cdReading=false;double cdReadStarted=0,cdReadFinished=-1;std::string cdReadDrive;
    bool cdEjecting=false;json cdEjectResult=json::object();bool controllerActive=false;std::string controllerModel;
    bool archive=false;json archiveLog=json::array(),beefweb=json::object();
};
class Ui {
    struct Hit {std::string key,action;json args;D2D1_RECT_F rect;bool disabled=false;std::string input;bool keyboardOnly=false;D2D1_RECT_F clip{-1e9f,-1e9f,1e9f,1e9f};};
    struct Input {HWND handle=nullptr;std::wstring initial;bool dirty=false,visible=false,cueSet=false,focused=false,password=false;float focusAmount=0,focusFrom=0;double focusChanged=0;
        RECT position{},region{};Com<ID2D1Bitmap1> bitmap;bool bitmapDirty=true,caretHidden=false,caretOn=false;DWORD selectionStart=0,selectionEnd=0;int firstVisibleLine=-1;POINT caretPosition{-1,-1};double caretChanged=0;D2D1_RECT_F caretRect{};};
    enum class ScrollArea{none,panel,detail};
    struct Scrollbar{D2D1_RECT_F rail{},thumb{};float maximum=0,travel=0,viewport=0;bool visible=false;};
    Scrollbar panelScrollbar_,detailScrollbar_;ScrollArea scrollbarDrag_=ScrollArea::none;float scrollbarGrab_=0;
    bool measuring_=false;Com<ID2D1Bitmap1> measurementTarget_;std::string panelMeasureKey_,detailMeasureKey_;float detailPaintScroll_=0,panelPaintScroll_=0;
    bool playPauseInitialized_=false,playPauseTarget_=false;float playPauseAmount_=0,playPauseFrom_=0,playPauseShapeAmount_=-1;double playPauseChanged_=0;Com<ID2D1GeometryGroup> playPauseShape_;json playPauseVertices_=json::array(),playPauseEvidence_=json::object(),searchEvidence_=json::object();
    std::map<std::wstring,std::wstring> searchCache_;
    json editorFocusEvents_=json::array();
    using Reel=NativeTitle;
    struct CachedText {Com<IDWriteTextLayout> layout;DWRITE_TEXT_METRICS metrics{};};
    HWND window_;Renderer* renderer_;Images* images_;std::function<void(std::string,json)> emit_;HFONT editorFont_=nullptr;HBRUSH editorBrush_=nullptr;COLORREF editorBackground_=0,editorForeground_=0;float editorFontScale_=0;
    std::vector<Hit> hits_;std::map<std::string,Input> inputs_;std::map<std::pair<int,int>,Com<IDWriteTextFormat>> formats_;std::map<std::string,std::pair<uint64_t,Com<ID2D1Bitmap>>> thumbnails_;
    Com<ID2D1GeometryGroup> veilMask_;std::string veilMaskKey_;
    Com<ID2D1SolidColorBrush> brush_;float width_=1920,height_=1080,scale_=1,scroll_=0,contentHeight_=0,detailScroll_=0,detailContentHeight_=0,detailPaneTop_=170,detailPaneBottom_=900,detailTabsTop_=0;bool detailTabsPinned_=false,detailClipActive_=false;D2D1_RECT_F detailHitClip_{};std::string lastPanel_,focus_,pressed_,highlightTrack_;double highlightStarted_=0;bool highlightPending_=false;
    std::map<std::wstring,CachedText> textCache_;
    std::string notificationIdentity_;double notificationStarted_=-1;float notificationScroll_=0,notificationScrollMax_=0,notificationHold_=0,notificationDownY_=0,notificationDragStart_=0;
    bool notificationVisible_=false,notificationPointer_=false;D2D1_RECT_F notificationBounds_{};json notificationEvidence_=json::object();
    std::map<std::string,D2D1_RECT_F> thumbnailSources_;
    NativeRuler ruler_;D2D1_RECT_F rulerRect_{};unsigned rulerNavigationSerial_=0;bool rulerDragging_=false,cardDragging_=false,viewerDragging_=false;float cardPointerX_=0,cardPointerY_=0;
    bool compact_=false,portrait_=false,focusVisible_=true;
    bool trackNavigation_=false,trackFocusRequested_=false;int pendingTrackSteps_=0;std::string lastTrackFocus_;
    float controlLeft_=0,controlRight_=0,panelOpacity_=0,panelFrom_=0,panelOffset_=12,panelHeight_=0;
    double panelChanged_=0;bool panelOpening_=false,panelClipActive_=false;
    std::string visiblePanel_;D2D1_RECT_F panelBounds_{},panelClip_{};
    struct Dropdown {bool open=false,visible=false;Hit source;int selected=0,first=0;D2D1_RECT_F bounds{};float progress=0,from=0,highlight=0,highlightFrom=0,highlightTarget=0;double changed=0,highlightChanged=0;} dropdown_;
    Com<ID2D1Bitmap1> backdrop_,panelShadowMask_;Com<ID2D1Effect> backdropBlur_,panelShadow_;
    std::map<std::string,Com<ID2D1Bitmap1>> badgeMasks_;json badgeEvidence_=json::array(),panelNodes_=json::object();
    std::map<std::string,Com<ID2D1Effect>> badgeEffects_;
    struct TechClock{double elapsed=0,seen=-100,appeared=0;bool playing=false;};std::map<std::string,TechClock> techClocks_;
    struct BadgeClock{double started=0,seen=-100;std::string identity;};std::map<std::string,BadgeClock> badgeClocks_;float haloMaskWidth_=0;
    float tabTarget_=0,tabFrom_=0;double tabChanged_=0;
    bool redactText_=false;std::vector<D2D1_RECT_F> redactions_;double decryptionVisibleSince_=0;
    std::map<std::string,Com<ID2D1SvgDocument>> vectors_;std::map<std::string,Com<ID2D1LinearGradientBrush>> gradients_;std::map<std::string,Com<ID2D1RadialGradientBrush>> radialGradients_;Spring themePill_{},tabPill_{},detailScrollMotion_{};std::string detailIdentity_;double frameTime_=0,fpsStarted_=0;unsigned fpsFrames_=0,fps_=0;float frameDt_=1.f/60;
    float pointerX_=-1000,pointerY_=-1000,downX_=0,downY_=0,scrollStart_=0;double downTime_=0;bool dragging_=false,dragCancel_=false,sliderDrag_=false;std::string sliderAction_;D2D1_RECT_F sliderRect_{};Hit dragHit_;double seekPreview_=-1;
    bool playbackVisible_=false;float playbackOpacity_=0;double playbackStarted_=0,playbackCoverChanged_=0;std::string playbackAlbumKey_;json currentPlaybackAlbum_=json::object(),previousPlaybackAlbum_=json::object();
    float headerTrackFrom_=0,headerTrackTarget_=0;double headerTrackChanged_=0;
    Reel browseTitle_,detailTitle_;std::map<std::string,Reel> textReels_;json layoutNodes_=json::object();D2D1_COLOR_F ink_{},muted_{},accent_{},panelColor_{};UiModel* model_=nullptr;
    IDWriteTextFormat* font(float size,int weight=400);
    float text(std::wstring value,D2D1_RECT_F rect,float size,int weight=400,D2D1_COLOR_F color={},bool wrap=false,bool center=false,float tracking=0,float lineHeight=1.4f);
    void line(float x1,float y1,float x2,float y2,D2D1_COLOR_F color,float thickness=1);
    void notification();
    void measure(const std::function<void()>& layout);
    void playPauseGlyph(const std::string& key,D2D1_RECT_F rect,D2D1_COLOR_F paint);
    void editor(Input& item,D2D1_RECT_F rect,D2D1_RECT_F clip);
    void scrollbar(ScrollArea area,D2D1_RECT_F rail,float viewport,float content);
    void scrollTo(ScrollArea area,float value,bool manual=true);
    float scrollMaximum(ScrollArea area)const;
    std::wstring searchNormalized(const std::wstring& text);
    void fill(D2D1_RECT_F rect,D2D1_COLOR_F color,float radius=0);
    void gradient(const std::string& key,D2D1_RECT_F rect,bool vertical,std::initializer_list<D2D1_GRADIENT_STOP> stops,float opacity=1);
    void vector(const std::string& key,D2D1_RECT_F rect,const std::string& source,D2D1_COLOR_F paint,float rotation=0);
    void logo(const std::string& name,D2D1_RECT_F rect,D2D1_COLOR_F paint);
    void ensureBadgeMask();void audioBadge(const std::string& name,D2D1_RECT_F rect,bool track);float albumBadges(const json& album,float left,float right,float top,float size);
    float sonyBadge(const std::string& name,D2D1_RECT_F rect,const std::wstring& caption,bool playing);
    float badgePhase(const std::string& name,bool track,double duration);
    bool selectedCd()const;D2D1_RECT_F cdEjectRect()const;void cdEject();void cdReading(float left,float right,float top,bool iconOnly=false);void drawDropdown();void setDropdownOpen(bool open);void backControl(std::string key,const std::wstring& label,float left,float top,std::string action="back");void panelBackdrop();void preparePanelShadow(float width,float height);
    void panelButton(std::string key,std::wstring label,D2D1_RECT_F rect,std::string action,json args=json::object(),bool primary=false,bool disabled=false,float size=11);
    float textWidth(const std::wstring& label,float size,int weight=400);
    float textHeight(const std::wstring& label,float size,float width,float lineHeight);
    void panelNode(const std::string& name,D2D1_RECT_F rect){panelNodes_[name]=json::array({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top});}
    float paragraph(std::wstring label,float left,float right,float y,float size=11,float lineHeight=1.9f);
    void selectControl(std::string key,D2D1_RECT_F rect,const json& values,const json& current,std::string group="preferences",bool disabled=false);
    void drawRedactions();void veils();void statistics();void credits(const json& album,float left,float right,float& y);
    void button(std::string key,std::wstring label,D2D1_RECT_F rect,std::string action,json args=json::object(),bool active=false,bool disabled=false);
    void input(std::string key,std::wstring label,D2D1_RECT_F rect,std::wstring initial,bool multiline=false,bool password=false,bool disabled=false);
    void toggle(std::string key,std::wstring label,float& y,bool value,bool disabled=false,std::wstring note=L"");
    void choice(std::string key,std::wstring label,float& y,const json& values,const json& current,std::string group="preferences",std::wstring note=L"",bool disabled=false);
    void slider(std::string key,std::wstring label,float& y,double value,double min=0,double max=1,double step=.01,bool disabled=false,std::string group="preferences");
    void title(Reel& reel,const std::string& value,D2D1_RECT_F rect,float size);
    void drawReel(Reel& reel,D2D1_RECT_F rect,D2D1_COLOR_F paint);
    float movingText(const std::string& key,const std::wstring& value,D2D1_RECT_F rect,float size,int weight,D2D1_COLOR_F paint,float spacing=0,float lineHeight=1.4f,bool wrap=false);
    void flatButton(std::string key,std::wstring label,D2D1_RECT_F rect,std::string action,json args,float size,D2D1_COLOR_F paint);
    void layoutNode(const std::string& key,D2D1_RECT_F rect){layoutNodes_[key]=json::array({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top});}
    void header();void browse();void drawRuler(float x,float y);void detail();void playback();void panel();void settings(float left,float right,float& y);void library(float left,float right,float& y);void online(float left,float right,float& y);void search(float left,float right,float& y);
    void cover(const json& album,D2D1_RECT_F rect,float opacity=1);void focusHit(const Hit& hit);void activate(const Hit& hit);void sliderAt(const Hit& hit,float x);
    void resolveTrackFocus();
    void controllerGlyph(const std::string& name,D2D1_RECT_F rect,D2D1_COLOR_F paint);
    void controllerHints();void controllerHelp(float left,float right,float& y);
    json controllerPromptEvidence_=json::object();
    void viewer();
    void archiveHeader();void archiveBrowse();void archiveDetail();void archiveSearch(float left,float right,float& y);void beefweb(float left,float right,float& y);
public:
    json parityEvidence()const{json controls=json::object(),links=json::object();for(const auto& hit:hits_)if(hit.action=="link")links[hit.key]=hit.args.value("url",std::string());for(const auto& hit:hits_)if(hit.key!="panel-scrim"&&hit.key!="panel-body")controls[hit.key]=json::array({(hit.rect.left+hit.rect.right)*.5f*scale_,(hit.rect.top+hit.rect.bottom)*.5f*scale_});return {{"controlsPixels",controls},{"links",links},{"logicalWidth",width_},{"logicalHeight",height_},{"scale",scale_},{"sortMode",model_?model_->preferences.value("sortMode",std::string("genre")):std::string()}};}
    Ui(HWND window,Renderer& renderer,Images& images,std::function<void(std::string,json)> emit);
    ~Ui();
    void rebind(Renderer& renderer,Images& images);
    void detach(){playPauseShape_.Reset();measurementTarget_.Reset();for(auto& [key,item]:inputs_)item.bitmap.Reset();veilMask_.Reset();veilMaskKey_.clear();formats_.clear();textCache_.clear();thumbnails_.clear();thumbnailSources_.clear();vectors_.clear();gradients_.clear();badgeMasks_.clear();badgeEffects_.clear();haloMaskWidth_=0;backdrop_.Reset();backdropBlur_.Reset();panelShadowMask_.Reset();panelShadow_.Reset();brush_.Reset();renderer_=nullptr;}
    ScreenGlass prepare(UiModel& model,unsigned width,unsigned height);
    void draw();
    void pointer(float x,float y,bool down,bool up,bool cancel,double now);
    void wheel(float pixels);bool key(UINT code,bool shift,bool ctrl);bool action(Action action);
    void beginTrackNavigation(){trackNavigation_=trackFocusRequested_=focusVisible_=true;pendingTrackSteps_=0;focus_.clear();lastTrackFocus_.clear();}
    void endTrackNavigation(){trackNavigation_=trackFocusRequested_=false;pendingTrackSteps_=0;if(focus_.starts_with("track-"))focus_.clear();}
    bool moveTrackFocus(int direction);
    const std::string& focusedKey()const{return focus_;}
    json trackFocusEvidence()const;
    static LRESULT CALLBACK editorProcedure(HWND handle,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR,DWORD_PTR data);
    void fieldChanged(HWND handle);
    void fieldCommit(HWND handle);
    std::wstring field(const std::string& key)const;
    void clearField(const std::string& key){auto found=inputs_.find(key);if(found!=inputs_.end()&&found->second.handle){SetWindowTextW(found->second.handle,L"");found->second.dirty=true;}scroll_=0;}
    json fields()const;
    json inputEvidence()const;
    json metadataMotion()const{json result=json::object();if(!model_)return result;for(const auto& [key,reel]:textReels_){size_t moving=0;for(const auto& [id,slot]:reel.slots)if(!slot.roll.finished(model_->now))++moving;result[key]={{"durationMs",460},{"movingGlyphs",moving},{"glyphs",reel.slots.size()},{"reduced",model_->preferences.value("reduced",false)}};}return result;}
    json status()const{json intro=nullptr;for(const auto& hit:hits_)if(hit.key=="tab-intro")intro=json::array({(hit.rect.left+hit.rect.right)*.5f*scale_,(hit.rect.top+hit.rect.bottom)*.5f*scale_});return {{"editorFocusEvents",editorFocusEvents_},{"playPause",playPauseEvidence_},{"search",searchEvidence_},{"scrollbars",{{"panel",{{"visible",panelScrollbar_.visible},{"maximum",panelScrollbar_.maximum},{"travel",panelScrollbar_.travel},{"rail",{panelScrollbar_.rail.left,panelScrollbar_.rail.top,panelScrollbar_.rail.right,panelScrollbar_.rail.bottom}},{"thumb",{panelScrollbar_.thumb.left,panelScrollbar_.thumb.top,panelScrollbar_.thumb.right,panelScrollbar_.thumb.bottom}}}},{"detail",{{"visible",detailScrollbar_.visible},{"maximum",detailScrollbar_.maximum},{"travel",detailScrollbar_.travel},{"rail",{detailScrollbar_.rail.left,detailScrollbar_.rail.top,detailScrollbar_.rail.right,detailScrollbar_.rail.bottom}},{"thumb",{detailScrollbar_.thumb.left,detailScrollbar_.thumb.top,detailScrollbar_.thumb.right,detailScrollbar_.thumb.bottom}}}},{"dragging",scrollbarDrag_!=ScrollArea::none}}},{"panelPaintScroll",panelPaintScroll_},{"detailPaintScroll",detailPaintScroll_},{"detailPaneBottom",detailPaneBottom_},{"notification",notificationEvidence_},{"parity",parityEvidence()},{"panelNodes",panelNodes_},{"layoutNodes",layoutNodes_},{"metadataMotion",metadataMotion()},{"panelBounds",json::array({panelBounds_.left,panelBounds_.top,panelBounds_.right,panelBounds_.bottom})},{"panelOpacity",panelOpacity_},{"panelScroll",scroll_},{"dropdownOpen",dropdown_.open},{"dropdownAnimation",{{"visible",dropdown_.visible},{"progress",dropdown_.progress},{"highlight",dropdown_.highlight},{"target",dropdown_.highlightTarget},{"openMs",180},{"closeMs",140},{"selectionMs",120},{"timing","linear"}}},{"inputs",inputEvidence()},{"audioBadges",badgeEvidence_},{"focus",focus_},{"trackFocus",trackFocusEvidence()},{"controllerPrompts",controllerPromptEvidence_},{"detailScroll",detailScroll_},{"detailContentHeight",detailContentHeight_},{"tabsPinned",detailTabsPinned_},{"tabsTop",detailTabsTop_},{"detailPaneTop",detailPaneTop_},{"detailTab",model_?model_->preferences.value("detailTab",std::string("tracks")):std::string("tracks")},{"introTabCenterPixels",intro},{"locationTrack",highlightTrack_},{"locationPending",highlightPending_},{"panel",model_?model_->panel:std::string()}};}
    void locate(const std::string& track,double now){endTrackNavigation();focus_=lastTrackFocus_="track-"+track;focusVisible_=false;highlightTrack_=track;highlightStarted_=now;highlightPending_=true;}
    void cancelLocation(){highlightTrack_.clear();highlightPending_=false;}
    bool editing()const;
    HBRUSH editorColor(HDC context)const{SetTextColor(context,editorForeground_);SetBkColor(context,editorBackground_);return editorBrush_;}
};
}
