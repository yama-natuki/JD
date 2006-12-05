// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#include "bbslistview.h"
#include "bbslistadmin.h"

#include "dbtree/interface.h"

#include "cache.h"
#include "command.h"
#include "global.h"

using namespace BBSLIST;


// メインビュー

BBSListViewMain::BBSListViewMain( const std::string& url,
                                  const std::string& arg1, const std::string& arg2 )
    : BBSListViewBase( url, arg1, arg2 )
{
    BBSListViewBase::set_expand_collapse( true );
}



BBSListViewMain::~BBSListViewMain()
{
#ifdef _DEBUG    
    std::cout << "BBSListViewMain::~BBSListViewMain : " << get_url() << std::endl;
#endif

    save_xml( CACHE::path_xml_listmain() );
}


void BBSListViewMain::shutdown()
{
#ifdef _DEBUG    
    std::cout << "BBSListViewMain::shutdown\n";
#endif
    save_xml( CACHE::path_xml_listmain_bkup() );
}


//
// リロード
//
// 更新が終わったらBBSListViewMain::update_view()が呼ばれる
//
void BBSListViewMain::reload()
{
    DBTREE::download_bbsmenu();
    set_status( "loading..." );
    BBSLIST::get_admin()->set_command( "set_status", get_url(), get_status() );
}



//
// 表示
//
void BBSListViewMain::show_view()
{
#ifdef _DEBUG
    std::cout << "BBSListViewMain::show_view : " << get_url() << std::endl;
#endif    

    // 板一覧のxmlが空ならサーバから取得
    if( DBTREE::get_xml_bbsmenu().empty() ) reload();

    else update_view();

    BBSLIST::get_admin()->set_command( "set_tablabel", get_url(), "板" );
}


//
// アップデート
//
void BBSListViewMain::update_view()
{
    xml2tree( DBTREE::get_xml_bbsmenu() );
    set_status( std::string() );
    BBSLIST::get_admin()->set_command( "set_status", get_url(), get_status() );
}


//
// ポップアップメニュー取得
//
// SKELETON::View::show_popupmenu() を参照すること
//
Gtk::Menu* BBSListViewMain::get_popupmenu( const std::string& url )
{
    if( url.empty() ) return NULL;

    Gtk::Menu* popupmenu;
    std::list< Gtk::TreeModel::iterator > list_it = get_treeview().get_selected_iterators();
    if( list_it.size() == 1 ){
            int type = path2type( *( get_treeview().get_selection()->get_selected_rows().begin() ) );

            if( type == TYPE_DIR ) popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_dir" ) );
            else popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu" ) );
    }
    else popupmenu = dynamic_cast< Gtk::Menu* >( ui_manager()->get_widget( "/popup_menu_mul" ) );

    return popupmenu;
}


//
// 板リスト保存
//
void BBSListViewMain::save_xml( const std::string& file )
{
    if( get_ready_tree() ) CACHE::save_rawdata( file , tree2xml() );
}
