// ライセンス: GPL2

//#define _DEBUG
//#define _DEBUG_RESIZE_TAB
#include "jddebug.h"

#include "dragnote.h"
#include "tabnote.h"
#include "tablabel.h"

#include "config/globalconf.h"

#include "session.h"
#include "dndmanager.h"

#include <gtk/gtk.h>

using namespace SKELETON;

//////////////////////////////////////////
//
// gtknotebook.c( Revision 19311, 2008-01-06 ) より引用
//
// gtkのバージョンが上がったときに誤動作しないかどうか注意
// 

typedef enum
{
  DRAG_OPERATION_NONE,
  DRAG_OPERATION_REORDER,
  DRAG_OPERATION_DETACH
} GtkNotebookDragOperation;

typedef enum
{
  ARROW_NONE,
  ARROW_LEFT_BEFORE,
  ARROW_RIGHT_BEFORE,
  ARROW_LEFT_AFTER,
  ARROW_RIGHT_AFTER
} GtkNotebookArrow;

#define GTK_NOTEBOOK_PAGE(_glist_) ((GtkNotebookPage *)((GList *)(_glist_))->data)
#define GTK_NOTEBOOK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_NOTEBOOK, GtkNotebookPrivate))

typedef struct _GtkNotebookPrivate GtkNotebookPrivate;

struct _GtkNotebookPrivate
{
  gpointer group;
  gint  mouse_x;
  gint  mouse_y;
  gint  pressed_button;
  guint dnd_timer;
  guint switch_tab_timer;

  gint  drag_begin_x;
  gint  drag_begin_y;

  gint  drag_offset_x;
  gint  drag_offset_y;

  GtkWidget *dnd_window;
  GtkTargetList *source_targets;
  GtkNotebookDragOperation operation;
  GdkWindow *drag_window;
  gint drag_window_x;
  gint drag_window_y;
  GtkNotebookPage *detached_tab;

  guint32 timestamp;

  guint during_reorder : 1;
  guint during_detach  : 1;
  guint has_scrolled   : 1;
};

struct _GtkNotebookPage
{
  GtkWidget *child;
  GtkWidget *tab_label;
  GtkWidget *menu_label;
  GtkWidget *last_focus_child;	/* Last descendant of the page that had focus */

  guint default_menu : 1;	/* If true, we create the menu label ourself */
  guint default_tab  : 1;	/* If true, we create the tab label ourself */
  guint expand       : 1;
  guint fill         : 1;
  guint pack         : 1;
  guint reorderable  : 1;
  guint detachable   : 1;

  GtkRequisition requisition;
  GtkAllocation allocation;

  gulong mnemonic_activate_signal;
  gulong notify_visible_handler;
};


//////////////////////////////////////////
//
// gtknotebook.c ( Revision 19311, 2008-01-06 ) を参考にして作成した描画関係の関数


// 描画本体
const bool TabNotebook::paint( GdkEventExpose* event )
{
    GtkNotebook *notebook = gobj();
    if( ! notebook || ! notebook->cur_page || ! GTK_WIDGET_VISIBLE( notebook->cur_page->child ) ) return Notebook::on_expose_event( event );

    GtkWidget *widget = GTK_WIDGET( notebook );
    GdkRectangle *area = &event->area;
    const Gdk::Rectangle rect( area );
    const Glib::RefPtr< Gdk::Window > win = get_window();

    widget->style->ythickness = m_ythickness;

    if( ! notebook->first_tab ) notebook->first_tab = notebook->children;

    GtkNotebookPage* page = NULL;
    if( ! GTK_WIDGET_MAPPED( notebook->cur_page->tab_label ) ) page = GTK_NOTEBOOK_PAGE( notebook->first_tab );
    else page = notebook->cur_page;

    // ビュー領域の枠の描画
    m_parent->draw_box( this, event );

    // タブの描画
    bool show_arrow = FALSE;
    GList *children = notebook->children;
    while( children ){

        page = ( GtkNotebookPage* ) children->data;
        children = children->next;

        if( ! GTK_WIDGET_VISIBLE( page->child ) ) continue;

        if( ! GTK_WIDGET_MAPPED( page->tab_label ) ) show_arrow = TRUE;

        else draw_tab( notebook, page, area, rect, win );
    }

    //矢印マーク描画
    if( show_arrow ){

        draw_arrow( widget, notebook, rect, win, ARROW_LEFT_BEFORE );
        draw_arrow( widget, notebook, rect, win, ARROW_RIGHT_AFTER );
    }

    // タブの中のwidgetの描画
    children = notebook->children;
    while( children ){

        GtkNotebookPage* page = ( GtkNotebookPage* ) children->data;
        children = children->next;

        if( page->tab_label->window == event->window &&
            GTK_WIDGET_DRAWABLE (page->tab_label))
            gtk_container_propagate_expose( GTK_CONTAINER( notebook ),
                                            page->tab_label, event );
    }

    widget->style->ythickness = 0;

    return true;
}


// タブ描画
void TabNotebook::draw_tab( const GtkNotebook *notebook,
                            const GtkNotebookPage *page,
                            GdkRectangle *area,
                            const Gdk::Rectangle& rect,
                            const Glib::RefPtr< Gdk::Window >& win

    )
{
    GdkRectangle child_area;
    GdkRectangle page_area;

    page_area.x = page->allocation.x;
    page_area.y = page->allocation.y;
    page_area.width = page->allocation.width;
    page_area.height = page->allocation.height - m_ythickness;

    if( gdk_rectangle_intersect( &page_area, area, &child_area ) )
    {
        Gtk::StateType state_type;
        if( notebook->cur_page == page ) state_type = Gtk::STATE_NORMAL;
        else state_type = Gtk::STATE_ACTIVE;

        get_style()->paint_extension( win,
                                      state_type,
                                      Gtk::SHADOW_OUT,
                                      rect,
                                      *this,
                                      "tab",
                                      page_area.x,
                                      page_area.y,
                                      page_area.width,
                                      page_area.height,
                                      Gtk::POS_BOTTOM
            );
    }
}


// 矢印(スクロール)マークの描画
void TabNotebook::draw_arrow( GtkWidget *widget,
                              const GtkNotebook *notebook,
                              const Gdk::Rectangle& rect,
                              const Glib::RefPtr< Gdk::Window >& win,
                              const int nbarrow )
{
    Gtk::StateType state_type;
    Gtk::ShadowType shadow_type;
    Gtk::ArrowType arrow;
    GdkRectangle arrow_rect;

    const bool before = ( nbarrow == ARROW_LEFT_BEFORE ? true : false );

    get_arrow_rect( widget, notebook, &arrow_rect, before );

    if( ( int ) notebook->in_child == nbarrow ){

        if( ( int ) notebook->click_child == nbarrow ) state_type = Gtk::STATE_ACTIVE;
        else state_type = Gtk::STATE_PRELIGHT;
    }
    else state_type = get_state();

    if( ( int ) notebook->click_child == nbarrow ) shadow_type = Gtk::SHADOW_IN;
    else shadow_type = Gtk::SHADOW_OUT;

    const int page = get_current_page();
    if( ( nbarrow == ARROW_LEFT_BEFORE && page == 0 )
        || ( nbarrow == ARROW_RIGHT_AFTER && page == get_n_pages() -1  )
        ){
        shadow_type = Gtk::SHADOW_ETCHED_IN;
        state_type = Gtk::STATE_INSENSITIVE;
    }

    if( nbarrow == ARROW_LEFT_BEFORE ) arrow = Gtk::ARROW_LEFT;
    else arrow = Gtk::ARROW_RIGHT;
 
    get_style()->paint_arrow( win,
                              state_type,
                              shadow_type, 
                              rect,
                              *this,
                              "notebook",
                              arrow,
                              TRUE,
                              arrow_rect.x,
                              arrow_rect.y, 
                              arrow_rect.width,
                              arrow_rect.height
        );
}


// 矢印マークの位置、幅、高さを取得
// before : true ならタブの左側に表示される矢印
void TabNotebook::get_arrow_rect( GtkWidget *widget, const GtkNotebook *notebook, GdkRectangle *rectangle, const gboolean before )
{
    GdkRectangle event_window_pos;
    if( get_event_window_position( widget, notebook, &event_window_pos ) ){

#if GTKMMVER >= 291
        gtk_widget_style_get( widget,
                              "scroll-arrow-hlength", &rectangle->width,
                              "scroll-arrow-vlength", &rectangle->height,
                              NULL );
#else
        // gtk+-2.9.1より前は 12 で固定
        rectangle->width = rectangle->height = 12;
#endif

        if( before ) rectangle->x = event_window_pos.x;
        else rectangle->x = event_window_pos.x + event_window_pos.width - rectangle->width;

        rectangle->y = event_window_pos.y + ( event_window_pos.height - rectangle->height ) / 2;
    }
}


// タブ描画領域の位置、幅、高さを取得
const gboolean TabNotebook::get_event_window_position( const GtkWidget *widget, const GtkNotebook *notebook, GdkRectangle *rectangle )
{
    GtkNotebookPage* visible_page = NULL;
    GList* children = notebook->children;
    while( children ){

        GtkNotebookPage* page = ( GtkNotebookPage* ) children->data;
        children = children->next;
        if( GTK_WIDGET_VISIBLE( page->child ) ){

            visible_page = page;
            break;
	}
    }

    if( visible_page ){

        const gint border_width = get_border_width();

        rectangle->x = widget->allocation.x + border_width;
        rectangle->y = widget->allocation.y + border_width;
        rectangle->width = widget->allocation.width - 2 * border_width;
        rectangle->height = visible_page->requisition.height;

        return TRUE;
    }

    return FALSE;
}



//////////////////////////////////////////



// ダミーWidgetを作成してtabにappend ( 表示はされない )
class DummyWidget : public Gtk::Widget
{
public:
    DummyWidget() : Gtk::Widget(){ set_flags(Gtk::NO_WINDOW); }
    virtual ~DummyWidget(){}
};


//////////////////////////////////////////////


TabNotebook::TabNotebook( DragableNoteBook* parent )
    : Gtk::Notebook(),
      m_parent( parent ),
      m_fixtab( false )
{
    m_layout_tab = create_pango_layout( "" );

    set_border_width( 0 );
    set_size_request( 1, -1 ); // これが無いと最大化を解除したときにウィンドウが勝手にリサイズする

    add_events( Gdk::POINTER_MOTION_MASK );
    add_events( Gdk::LEAVE_NOTIFY_MASK );

    // DnD設定
    // ドロップ側に設定する
    drag_source_unset();
    drag_dest_unset();
    std::list< Gtk::TargetEntry > targets;
    targets.push_back( Gtk::TargetEntry( DNDTARGET_TAB, Gtk::TARGET_SAME_APP, 0 ) );
    drag_dest_set( targets, Gtk::DEST_DEFAULT_MOTION | Gtk::DEST_DEFAULT_DROP );

    Glib::RefPtr< Gtk::RcStyle > rcst = get_modifier_style();
    Glib::RefPtr< Gtk::Style > st = get_style();

    m_tab_mrg = rcst->get_xthickness() * 2;
    if( m_tab_mrg <= 0 ) m_tab_mrg = st->get_xthickness() * 2;

    m_ythickness = rcst->get_ythickness();
    if( m_ythickness <= 0 ) m_ythickness = st->get_ythickness();

    if( m_ythickness > 0 ){
        rcst->set_ythickness( 0 );
        modify_style( rcst );
        property_tab_vborder() = property_tab_vborder() + m_ythickness*2;
    }

    m_pre_width = get_width();
}

//
// クロック入力
//
void TabNotebook::clock_in()
{
    // Gtk::NoteBook は configure_event()をキャッチ出来ないので
    // 応急処置としてタイマーの中でサイズが変更したか調べて
    // 変わっていたらタブ幅を調整する
    if( ! m_fixtab && get_n_pages() && m_pre_width != get_width()
        && ! SESSION::is_booting() 
        && ! SESSION::is_quitting()
        ){

        calc_tabsize();
        adjust_tabwidth();
    }
}


int TabNotebook::append_tab( Widget& tab )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::append_tab\n";
#endif

    // ダミーWidgetを作成してtabにappend (表示はされない )
    // remove_tab()でdeleteする
    DummyWidget* dummypage = new DummyWidget();

    return append_page( *dummypage , tab );
}


int TabNotebook::insert_tab( Widget& tab, int page )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::insert_tab position = "  << page << std::endl;
#endif

    // ダミーWidgetを作成してtabにappend (表示はされない )
    // remove_tab()でdeleteする
    DummyWidget* dummypage = new DummyWidget();

    return insert_page( *dummypage, tab, page );
}


void TabNotebook::remove_tab( int page )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::remove_tab page = " << page << std::endl;
#endif

    // ダミーWidgetをdelete
    Gtk::Widget* dummypage = get_nth_page( page );
    remove_page( page );

    if( dummypage ) delete dummypage;

    adjust_tabwidth();
}


void TabNotebook::reorder_child( int page1, int page2 )
{
    Gtk::Notebook::reorder_child( *get_nth_page( page1 ), page2 );
}


// タブ取得
SKELETON::TabLabel* TabNotebook::get_tablabel( int page )
{
    return dynamic_cast< SKELETON::TabLabel* >( get_tab_label( *get_nth_page( page ) ) );
}


//
// マウスの下にあるタブの番号を取得
//
// タブ上では無いときは-1を返す
// マウスがタブの右側にある場合はページ数( get_n_pages() )を返す
//
const int TabNotebook::get_page_under_mouse()
{
    int x, y;
    Gdk::Rectangle rect = get_allocation();
    get_pointer( x, y );
    x += rect.get_x();
    y += rect.get_y();

#ifdef _DEBUG
    std::cout << "TabNotebook::get_page_under_mouse x = " << x << " y = " << y << std::endl;
#endif

    if( y < rect.get_y() || y > rect.get_y() + rect.get_height() ) return -1;

    calc_tabsize();

    const int pages = get_n_pages();
    int ret = pages;
    for( int i = 0; i < pages; ++i ){

        SKELETON::TabLabel* tab = get_tablabel( i );
        if( tab ){

            int tab_x = tab->get_tab_x();
            int tab_w = tab->get_tab_width();

            if( tab_x < 0 ) continue;

#ifdef _DEBUG
            std::cout << "page = " << i << " x = " << tab_x << " w = " << tab_w << std::endl;
#endif

            if( x < tab_x ){
                ret = -1;
                break;
            }
            if( x >= tab_x && x <= tab_x + tab_w ){
                ret = i;
                break;
            }
        }
    }

#ifdef _DEBUG
    std::cout << "ret = " << ret << std::endl;
#endif

    return ret;
}


//
// タブの文字列取得
//
const std::string TabNotebook::get_tab_fulltext( int page )
{
    SKELETON::TabLabel* tablabel = get_tablabel( page );
    if( ! tablabel ) return std::string();

    return tablabel->get_fulltext();
}



//
// タブに文字列をセットとタブ幅調整
//
void TabNotebook::set_tab_fulltext( const std::string& str, int page )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::set_tab_fulltext page = " << page << " " << str << std::endl;
#endif

    SKELETON::TabLabel* tablabel = get_tablabel( page );
    if( tablabel ){
        tablabel->set_fulltext( str );
        if( m_fixtab ) tablabel->resize_tab( str.length() );
        else adjust_tabwidth();
    }
}


//
// 各タブのサイズと座標を取得
//
void TabNotebook::calc_tabsize()
{
#ifdef _DEBUG
    std::cout << "TabNotebook::calc_tabsize\n";
#endif

    GtkNotebook *notebook = gobj();
    GList * children = notebook->children;

    for( int i = 0; children ; ++i, children = children->next ){

        GtkNotebookPage* page = ( GtkNotebookPage* ) children->data;
        SKELETON::TabLabel* tab = get_tablabel( i );
        if( tab ){

            int tab_x = -1;
            int tab_y = -1;
            int tab_w = -1;
            int tab_h = -1;

            if( tab->is_mapped() && page ){

                tab_x = page->allocation.x;
                tab_y = page->allocation.y;
                tab_w = page->allocation.width;
                tab_h = page->allocation.height;

                Gdk::Rectangle rect = tab->get_allocation();
                m_tab_mrg = tab_w - rect.get_width();
            }

#ifdef _DEBUG
            std::cout << "page = " << i << " x = " << tab_x << " w = " << tab_w << " mrg = " << m_tab_mrg << std::endl;
#endif
            tab->set_tab_x( tab_x );
            tab->set_tab_y( tab_y );
            tab->set_tab_width( tab_w );
            tab->set_tab_height( tab_h );
        }
    }
}


//
// タブ幅調整
//
#define LABEL_WIDTH ( ulabel.substr( 0,  vec_width[ i ] ) + ( vec_width[ i ] < vec_width_org[ i ] ? "..." : "" ) )

bool TabNotebook::adjust_tabwidth()
{
    // 起動中とシャットダウン中は処理しない
    if( SESSION::is_booting() ) return false;
    if( SESSION::is_quitting() ) return false;

    const int mrg_notebook = 30;

    const int pages = get_n_pages();
    if( ! pages ) return false;

    // layoutにラベルのフォントをセットする
    SKELETON::TabLabel* tab = get_tablabel( 0 );
    if( ! tab ) return false;
    m_layout_tab->set_font_description( tab->get_label_font_description() );

    std::vector< int > vec_width_org; // 変更前のタブの文字数
    std::vector< int > vec_width; // 変更後のタブの文字数
    vec_width_org.resize( pages );
    vec_width.resize( pages );

    m_pre_width = get_width();
    const int width_notebook = m_pre_width - mrg_notebook;

    const int avg_width_tab = (int)( (double)width_notebook / MAX( 3, pages+0.5 ) );  // タブ幅の平均値

#ifdef _DEBUG_RESIZE_TAB
    std::cout << "TabNotebook::adjust_tabwidth\n"
              << "width_notebook = " << width_notebook << " page = " << pages << std::endl
              << "avg_width_tab = " << avg_width_tab
              << " tab_mrg = " << m_tab_mrg
              << std::endl;
#endif

    // 一端、全てのタブの幅を平均値以下に縮める
    for( int i = 0; i < pages; ++i ){

        tab = get_tablabel( i );
        if( tab ){

            Glib::ustring ulabel( tab->get_fulltext() );
            vec_width_org[ i ] = vec_width[ i ] = ulabel.length();

            while( vec_width[ i ] > CONFIG::get_tab_min_str() ){

                m_layout_tab->set_text( LABEL_WIDTH );
                int width = m_layout_tab->get_pixel_ink_extents().get_width() + tab->get_margin() + m_tab_mrg;

#ifdef _DEBUG_RESIZE_TAB
                std::cout << "s " << i << " " << width << " / " << avg_width_tab
                          << " lng = " << vec_width[ i ] << " : " << ulabel.substr( 0, vec_width[ i ] ) << std::endl;
#endif

                if( width < avg_width_tab ) break;
                --vec_width[ i ];
                if( vec_width[ i ] < 0 ) vec_width[ i ] = 0;
            }
        }
    }

    // 横をはみださないようにタブ幅を延ばしていく
    int width_total = 0;
    for( int i = 0; i < pages; ++i ){

        SKELETON::TabLabel* tab = get_tablabel( i );
        if( tab ){

            Glib::ustring ulabel( tab->get_fulltext() );

            for(;;){

                if( vec_width[ i ] >= vec_width_org[ i ] ) break;

                ++vec_width[ i ];

                m_layout_tab->set_text( LABEL_WIDTH );
                int width = m_layout_tab->get_pixel_ink_extents().get_width() + tab->get_margin() + m_tab_mrg;

#ifdef _DEBUG_RESIZE_TAB
                std::cout << "w " << i << " " << width << " / " << avg_width_tab
                          << " total= " << width_total + width << " / " << avg_width_tab * ( i + 1 )
                          << " lng = " << vec_width[ i ] << " : " << ulabel.substr( 0, vec_width[ i ] ) << std::endl;
#endif
                // 最大値を越えたらひとつ戻してbreak;
                if( width_total + width > avg_width_tab * ( i + 1 ) ){
                    --vec_width[ i ];
                    break;
                }
            }

            m_layout_tab->set_text( LABEL_WIDTH );
            width_total += ( m_layout_tab->get_pixel_ink_extents().get_width() + tab->get_margin() + m_tab_mrg );

            tab->resize_tab( vec_width[ i ] );
        }
    }

    // 枠の再描画
    m_parent->queue_draw();

    return true;
}


//
// タブの高さ、幅、位置を取得 ( 描画用 )
//
void TabNotebook::get_alloc_tab( Alloc_NoteBook& alloc )
{
    alloc.x_tab = 0;
    alloc.width_tab = 0;
    alloc.height_tab = 0;

    GtkNotebook *notebook = gobj();
    if( notebook && notebook->cur_page ){

        const int bw = get_border_width();
        const int xx = get_allocation().get_x() + bw;

        alloc.x_tab = notebook->cur_page->allocation.x - xx;
        alloc.width_tab = notebook->cur_page->allocation.width;

        // タブの高さを m_ythickness 分低くする
        // draw_tab() を参照
        alloc.height_tab = notebook->cur_page->allocation.height - m_ythickness; 
    }
}


//
// 描画イベント
//
bool TabNotebook::on_expose_event( GdkEventExpose* event )
{
    return paint( event );
}



// signal_button_press_event と signal_button_release_event は emit されない
// ときがあるので自前でemitする
bool TabNotebook::on_button_press_event( GdkEventButton* event )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::on_button_press_event\n";
#endif

    if( m_sig_button_press.emit( event ) ) return true;

#ifdef _DEBUG
    std::cout << "Gtk::Notebook::on_button_press_event\n";
#endif

    return Gtk::Notebook::on_button_press_event( event );
}


bool TabNotebook::on_button_release_event( GdkEventButton* event )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::on_button_release_event\n";
#endif

    if( m_sig_button_release.emit( event ) ) return true;

#ifdef _DEBUG
    std::cout << "Gtk::Notebook::on_button_release_event\n";
#endif

    return Gtk::Notebook::on_button_release_event( event );
}


//
// マウスが動いた
//
bool TabNotebook::on_motion_notify_event( GdkEventMotion* event )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::on_motion_notify_event\n";
#endif

    m_sig_tab_motion_event.emit();

    return Gtk::Notebook::on_motion_notify_event( event );
}


//
// マウスが出た
//
bool TabNotebook::on_leave_notify_event( GdkEventCrossing* event )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::leave\n";
#endif

    m_sig_tab_leave_event.emit();

    return Gtk::Notebook::on_leave_notify_event( event );
}


//
// マウスホイールを回した
//
bool TabNotebook::on_scroll_event( GdkEventScroll* event )
{
#ifdef _DEBUG
    std::cout << "TabNotebook::scroll\n";
#endif

    m_sig_scroll_event.emit( event );

    return Gtk::Notebook::on_scroll_event( event );
}


//
// ドラッグ中にマウスを動かした
//
bool TabNotebook::on_drag_motion( const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
#ifdef _DEBUG
    std::cout << "Gtk::Notebook::on_drag_motion x = " << x << " y = " << y << std::endl;
#endif

    int tab_x = -1;
    int tab_y = -1;
    int tab_w = -1;

    int page = get_page_under_mouse();
    if( page >= 0 ){

        if( page >= get_n_pages() ) page = get_n_pages() -1;

        SKELETON::TabLabel* tab = get_tablabel( page );
        if( tab ){

            tab_x = tab->get_tab_x();
            tab_y = tab->get_tab_y();
            tab_w = tab->get_tab_width();

#ifdef _DEBUG
            std::cout << "page = " << page
                      << " tab_x = " << tab_x << " tab_y = " << tab_y << " tab_w " << tab_w << std::endl;
#endif
        }
    }

    m_sig_tab_drag_motion( page, tab_x, tab_y, tab_w );

    // on_drag_motion をキャンセルしないとDnD中にタブが勝手に切り替わる( gtknotebook.c をハック )
    return true;
}
