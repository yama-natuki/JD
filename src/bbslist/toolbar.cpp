// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#include "toolbar.h"
#include "bbslistadmin.h"

#include "skeleton/view.h"
#include "skeleton/menubutton.h"
#include "skeleton/imgtoolbutton.h"

#include "control/controlutil.h"
#include "control/controlid.h"

#include "icons/iconmanager.h"

#include "command.h"
#include "session.h"
#include "compmanager.h"
#include "global.h"

using namespace BBSLIST;


BBSListToolBar::BBSListToolBar() :
    SKELETON::ToolBar( BBSLIST::get_admin() ),
    m_button_toggle( "板一覧とお気に入りの切り替え", true, true, m_label ),
    m_button_check_update_root( NULL ),
    m_button_check_update_open_root( NULL )
{
    m_button_toggle.get_button()->set_tooltip_arrow( "板一覧とお気に入りの切り替え\n\nマウスホイール回転でも切り替え可能" );

    m_label.set_alignment( Gtk::ALIGN_LEFT );
    std::vector< std::string > menu;
    menu.push_back( "板一覧" );
    menu.push_back( "お気に入り" );
    m_button_toggle.get_button()->append_menu( menu );
    m_button_toggle.get_button()->signal_selected().connect( sigc::mem_fun(*this, &BBSListToolBar::slot_toggle ) );
    m_button_toggle.get_button()->signal_scroll_event().connect(  sigc::mem_fun( *this, &BBSListToolBar::slot_scroll_event ));
    m_button_toggle.get_button()->set_enable_sig_clicked( false );

#if GTKMMVER >= 2120
    m_tool_label.set_icon_size( Gtk::ICON_SIZE_MENU );
#endif
    m_tool_label.set_toolbar_style( Gtk::TOOLBAR_ICONS );
    m_tool_label.append( m_button_toggle );
    m_tool_label.append( *get_button_close() );
    pack_start( m_tool_label, Gtk::PACK_SHRINK );

    pack_buttons();
    add_search_control_mode( CONTROL::MODE_BBSLIST );
}


//
// ボタンのパッキング
//
// virtual
void BBSListToolBar::pack_buttons()
{
    int num = 0;
    for(;;){
        int item = SESSION::get_item_sidebar( num );
        if( item == ITEM_END ) break;
        switch( item ){

            case ITEM_SEARCHBOX:
                get_buttonbar().append( *get_tool_search( CORE::COMP_SEARCH_BBSLIST ) );
                break;

            case ITEM_CHECK_UPDATE_ROOT:
                if( ! m_button_check_update_root ){
                    m_button_check_update_root = Gtk::manage( new SKELETON::ImgToolButton( Gtk::Stock::REFRESH ) );
                    m_button_check_update_root->signal_clicked().connect( sigc::mem_fun(*this, &BBSListToolBar::slot_check_update_root ) );
                    set_tooltip( *m_button_check_update_root, CONTROL::get_label_motions( CONTROL::CheckUpdateRoot ) );
                }
                get_buttonbar().append( *m_button_check_update_root );
                break;

            case ITEM_CHECK_UPDATE_OPEN_ROOT:
                if( ! m_button_check_update_open_root ){
                    m_button_check_update_open_root = Gtk::manage( new SKELETON::ImgToolButton( ICON::THREAD ) );
                    m_button_check_update_open_root->signal_clicked().connect( sigc::mem_fun(*this, &BBSListToolBar::slot_check_update_open_root ) );
                    set_tooltip( *m_button_check_update_open_root, CONTROL::get_label_motions( CONTROL::CheckUpdateOpenRoot ) );
                }
                get_buttonbar().append( *m_button_check_update_open_root );
                break;

            case ITEM_SEARCH_NEXT:
                get_buttonbar().append( *get_button_down_search() );
                break;

            case ITEM_SEARCH_PREV:
                get_buttonbar().append( *get_button_up_search() );
                break;

            case ITEM_SEPARATOR:
                pack_separator();
                break;
        }
        ++num;
    }

    set_relief();
    show_all_children();
}



// タブが切り替わった時にDragableNoteBook::set_current_toolbar()から呼び出される( Viewの情報を取得する )
// virtual
void BBSListToolBar::set_view( SKELETON::View* view )
{
    ToolBar::set_view( view );

    if( view )  m_label.set_text( view->get_label() );
}


void BBSListToolBar::slot_toggle( int i )
{
#ifdef _DEBUG 	 
     std::cout << "BBSListToolBar::slot_toggle = " << get_url() << " i = " << i << std::endl;
#endif 	 
  	 
     switch( i ){
  	 
         case 0:
             if( get_url() != URL_BBSLISTVIEW ) CORE::core_set_command( "switch_sidebar", URL_BBSLISTVIEW ); 	 
             break; 	 
  	 
         case 1:
             if( get_url() != URL_FAVORITEVIEW ) CORE::core_set_command( "switch_sidebar", URL_FAVORITEVIEW ); 	 
             break; 	 
     }
}


bool BBSListToolBar::slot_scroll_event( GdkEventScroll* event )
{
    guint direction = event->direction;

#ifdef _DEBUG
    std::cout << "BBSListToolBar::slot_scroll_event dir = " << direction << std::endl;
#endif

    if( direction == GDK_SCROLL_UP ) slot_toggle( 0 );
    if( direction == GDK_SCROLL_DOWN ) slot_toggle( 1 );

    return true;
}


void BBSListToolBar::slot_check_update_root()
{
    CORE::core_set_command( "check_update_root", "" );
}


void BBSListToolBar::slot_check_update_open_root()
{
    CORE::core_set_command( "check_update_open_root", "" );
}



////////////////////////////////////////


EditListToolBar::EditListToolBar() :
    SKELETON::ToolBar( NULL )
{
    pack_buttons();
}


//
// ボタンのパッキング
//
// virtual
void EditListToolBar::pack_buttons()
{
    get_buttonbar().append( *get_tool_search( CORE::COMP_SEARCH_BBSLIST ) );
    get_buttonbar().append( *get_button_down_search() );
    get_buttonbar().append( *get_button_up_search() );
    add_search_control_mode( CONTROL::MODE_BBSLIST );

    get_buttonbar().append( *get_button_undo() );
    get_buttonbar().append( *get_button_redo() );
    get_buttonbar().append( *get_button_close() );

    set_relief();
    show_all_children();
}
