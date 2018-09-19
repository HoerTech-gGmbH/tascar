#include "tascar_mainwindow.h"
#include "../build/tascar_xy.xpm"
#include "../build/tascar_xz.xpm"
#include "../build/tascar_yz.xpm"
#include "../build/tascar_xyz.xpm"
#include "logo.xpm"
#include "pdfexport.h"
#include <fstream>

#define GET_WIDGET(x) m_refBuilder->get_widget(#x,x);if( !x ) throw TASCAR::ErrMsg(std::string("No widget \"")+ #x + std::string("\" in builder."))

void error_message(const std::string& msg)
{
  std::cerr << "Error: " << msg << std::endl;
  Gtk::MessageDialog dialog("Error",false,Gtk::MESSAGE_ERROR);
  dialog.set_secondary_text(msg);
  dialog.run();
}

tascar_window_t::tascar_window_t(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade)
  : Gtk::Window(cobject),
    m_refBuilder(refGlade),
    scene_map(NULL),
    statusbar_main(NULL),
    timeline(NULL),
    scene_selector(NULL),
    selected_scene(0),
    active_selector(NULL),
    active_object(NULL),
    active_label_sourceline(NULL),
    active_mixer(NULL),
    active_source_ctl(NULL),
    blink(false),
    sessionloaded(false),
    sessionquit(false)
{
  Gsv::init();
  language_manager = Gsv::LanguageManager::create();
  //std::vector<std::string> langs = language_manager->get_language_ids ();
  //std::cout << "number of languages found: " << langs.size () << std::endl;
  //
  //for (std::vector<std::string>::const_iterator iter = langs.begin(); iter != langs.end (); ++iter) {
  //  if (!(iter->empty())) {
  //    Glib::RefPtr<Gsv::Language> lang = language_manager->get_language (*iter);
  //    std::cout << "language: " << lang->get_name () << std::endl;
  //  } else {
  //    std::cout << "language: null" << std::endl;
  //  }
  //}
  pthread_mutex_init( &mtx_draw, NULL );
  m_refBuilder->get_widget("SceneDraw",scene_map);
  if( !scene_map )
    throw TASCAR::ErrMsg("No drawing area widget");
  con_draw = scene_map->signal_draw().connect(sigc::mem_fun(*this, &tascar_window_t::draw_scene), false);
  con_timeout = Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_window_t::on_timeout), 100 );
  con_timeout_blink = Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_window_t::on_timeout_blink), 600 );
  con_timeout_statusbar = Glib::signal_timeout().connect( sigc::mem_fun(*this, &tascar_window_t::on_timeout_statusbar), 100 );
  scene_map->add_events(Gdk::SCROLL_MASK);
  scene_map->add_events(Gdk::BUTTON_PRESS_MASK);
  scene_map->signal_scroll_event().connect( sigc::mem_fun(*this,&tascar_window_t::on_map_scroll));
  scene_map->signal_button_press_event().connect( sigc::mem_fun(*this,&tascar_window_t::on_map_clicked));
  m_refBuilder->get_widget("StatusbarMain",statusbar_main);
  if( !statusbar_main )
    throw TASCAR::ErrMsg("No main status bar");
  m_refBuilder->get_widget("Timeline",timeline);
  if( !timeline )
    throw TASCAR::ErrMsg("No time line");
  timeline->signal_value_changed().connect(sigc::mem_fun(*this,
                                                         &tascar_window_t::on_time_changed));
  m_refBuilder->get_widget_derived("RouteButtons",source_panel);
  if( !source_panel )
    throw TASCAR::ErrMsg("No source panel");
  Gtk::AboutDialog* aboutdialog(NULL);
  m_refBuilder->get_widget("aboutdialog",aboutdialog);
  if( aboutdialog )
    aboutdialog->set_logo(Gdk::Pixbuf::create_from_xpm_data(logo));
  GET_WIDGET(scene_selector);
  scene_selector->signal_changed().connect(sigc::mem_fun(*this, &tascar_window_t::on_scene_selector_changed));
  GET_WIDGET(notebook);
  GET_WIDGET(active_selector);
  GET_WIDGET(active_type_label);
  GET_WIDGET(active_track);
  GET_WIDGET(active_label_sourceline);
  GET_WIDGET(active_source_display);
  GET_WIDGET(active_mixer);
  active_selector->signal_changed().connect(sigc::mem_fun(*this, &tascar_window_t::on_active_selector_changed));
  active_track->signal_toggled().connect(sigc::mem_fun(*this, &tascar_window_t::on_active_track_changed));
  Gtk::Image* image_xy(NULL);
  m_refBuilder->get_widget("image_xy",image_xy);
  if( image_xy )
    image_xy->set(Gdk::Pixbuf::create_from_xpm_data(tascar_xy));
  Gtk::Image* image_xz(NULL);
  m_refBuilder->get_widget("image_xz",image_xz);
  if( image_xz )
    image_xz->set(Gdk::Pixbuf::create_from_xpm_data(tascar_xz));
  Gtk::Image* image_yz(NULL);
  m_refBuilder->get_widget("image_yz",image_yz);
  if( image_yz )
    image_yz->set(Gdk::Pixbuf::create_from_xpm_data(tascar_yz));
  Gtk::Image* image_xyz(NULL);
  m_refBuilder->get_widget("image_xyz",image_xyz);
  if( image_xyz )
    image_xyz->set(Gdk::Pixbuf::create_from_xpm_data(tascar_xyz));
  //GET_WIDGET(menu_osc_vars);
  GET_WIDGET(text_warnings);
  //GET_WIDGET(text_source);
  GET_WIDGET(scrolled_window_source);
  scrolled_window_source->add(source_view);
  source_view.set_show_line_numbers();
  source_view.set_editable(false);
  source_view.show();
  source_buffer = source_view.get_source_buffer();
  source_buffer->set_language(language_manager->get_language("xml"));
  //DEBUG(language_manager->get_language("xml"));
  source_buffer->set_highlight_syntax();
  GET_WIDGET(legal_view);
  GET_WIDGET(osc_vars);
  GET_WIDGET(text_srv_addr);
  GET_WIDGET(text_srv_port);
  //Glib::RefPtr<Gtk::TextBuffer::Tag> t_bold(legal_view->get_buffer()->create_tag("bold"));
  //t_bold->property_weight() = PANGO_WEIGHT_BOLD;
  
  //Create actions for menus and toolbars:
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupMain = Gio::SimpleActionGroup::create();
  refActionGroupMain->add_action("scene_new",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_new));
  refActionGroupMain->add_action("scene_open",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_open));
  refActionGroupMain->add_action("scene_open_example",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_open_example));
  refActionGroupMain->add_action("scene_reload",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_reload));
  refActionGroupMain->add_action("export_csv",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_exportcsv));
  refActionGroupMain->add_action("export_csvsounds",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_exportcsvsounds));
  refActionGroupMain->add_action("export_pdf",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_exportpdf));
  refActionGroupMain->add_action("export_acmodel",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_exportacmodel));
  refActionGroupMain->add_action("scene_close",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_close));
  refActionGroupMain->add_action("quit",sigc::mem_fun(*this, &tascar_window_t::on_menu_file_quit));
  refActionGroupMain->add_action("help_bugreport",sigc::mem_fun(*this, &tascar_window_t::on_menu_help_bugreport));
  refActionGroupMain->add_action("help_manual",sigc::mem_fun(*this, &tascar_window_t::on_menu_help_manual));
  refActionGroupMain->add_action("help_about",sigc::mem_fun(*this, &tascar_window_t::on_menu_help_about));
  insert_action_group("main",refActionGroupMain);
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupView = Gio::SimpleActionGroup::create();
  refActionGroupView->add_action("zoom_in",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_zoom_in));
  refActionGroupView->add_action("zoom_out",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_zoom_out));
  refActionGroupView->add_action("viewport_xy",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_xy));
  refActionGroupView->add_action("viewport_xz",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_xz));
  refActionGroupView->add_action("viewport_yz",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_yz));
  refActionGroupView->add_action("viewport_xyz",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_xyz));
  refActionGroupView->add_action("viewport_rotz",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_rotz));
  refActionGroupView->add_action("viewport_rotzcw",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_rotzcw));
  refActionGroupView->add_action("viewport_setref",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_viewport_setref));
  refActionGroupView->add_action("meter_rmspeak",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_meter_rmspeak));
  refActionGroupView->add_action("meter_rms",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_meter_rms));
  refActionGroupView->add_action("meter_peak",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_meter_peak));
  refActionGroupView->add_action("meter_percentile",sigc::mem_fun(*this, &tascar_window_t::on_menu_view_meter_percentile));
  insert_action_group("view",refActionGroupView);
  Glib::RefPtr<Gio::SimpleActionGroup> refActionGroupTransport = Gio::SimpleActionGroup::create();
  refActionGroupTransport->add_action("play",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_play));
  refActionGroupTransport->add_action("stop",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_stop));
  refActionGroupTransport->add_action("rewind",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_rewind));
  refActionGroupTransport->add_action("forward",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_forward));
  refActionGroupTransport->add_action("previous",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_previous));
  refActionGroupTransport->add_action("next",sigc::mem_fun(*this, &tascar_window_t::on_menu_transport_next));
  insert_action_group("transport",refActionGroupTransport);
}

bool tascar_window_t::on_timeout()
{
  if( session ){
    if( pthread_mutex_trylock( &mtx_draw ) == 0 ){
      Glib::RefPtr<Gdk::Window> win = scene_map->get_window();
      if (win){
        Gdk::Rectangle r(0,0, 
                         scene_map->get_allocation().get_width(),
                         scene_map->get_allocation().get_height() );
        win->invalidate_rect(r, true);
      }
      if( source_panel )
        source_panel->invalidate_win();
      if( active_source_ctl )
        active_source_ctl->invalidate_win();
      if( session )
        draw.set_time(session->tp_get_time());
      timeline->set_value(draw.get_time());
      pthread_mutex_unlock( &mtx_draw );
    }
  }
  if( sessionquit )
    hide();
  return true;
}

bool tascar_window_t::on_timeout_statusbar()
{
  if( pthread_mutex_trylock( &mtx_draw ) == 0 ){
    char cmp[1024];
    if( session && session->is_running() ){
      sprintf(cmp,"scenes: %ld  point sources: %d/%d  diffuse sources: %d/%d",
              (long int)(session->scenes.size()),
              session->get_active_pointsources(),session->get_total_pointsources(),
              session->get_active_diffusesources(),session->get_total_diffusesources());
      sessionloaded = true;
    }else{
      sprintf(cmp,"No session loaded.");
      if( sessionloaded )
        reset_gui();
      sessionloaded = false;
    }
    statusbar_main->remove_all_messages();
    statusbar_main->push(cmp);
    if( session && session->is_running() ){
      if( source_panel )
        source_panel->update();
      if( active_source_ctl )
        active_source_ctl->update();
    }
    pthread_mutex_unlock( &mtx_draw );
  }
  return true;
}

bool tascar_window_t::on_timeout_blink()
{
  if( pthread_mutex_trylock( &mtx_draw ) == 0 ){
    if( blink )
      blink = false;
    else
      blink = true;
    draw.set_blink(blink);
    pthread_mutex_unlock( &mtx_draw );
  }
  return true;
}

tascar_window_t::~tascar_window_t()
{
  con_draw.disconnect();
  con_timeout.disconnect();
  con_timeout_blink.disconnect();
  con_timeout_statusbar.disconnect();
  active_mixer->remove();
  if( active_source_ctl )
    delete active_source_ctl;
  pthread_mutex_trylock( &mtx_draw );
  pthread_mutex_unlock( &mtx_draw );
  pthread_mutex_destroy( &mtx_draw );
}

bool tascar_window_t::draw_scene(const Cairo::RefPtr<Cairo::Context>& cr)
{
  if( session ){
    if( session->lock_vars() ){
      if( session->is_running() ){
        if( pthread_mutex_trylock( &mtx_draw ) == 0 ){
          Gtk::Allocation allocation = scene_map->get_allocation();
          const int width = allocation.get_width();
          const int height = allocation.get_height();
          cr->rectangle(0,0,width,height);
          cr->clip();
          cr->translate(0.5*width, 0.5*height);
          double wscale(0.5*std::max(height,width));
          double markersize(5.0/wscale);
          cr->scale( wscale, wscale );
          cr->set_line_width( 0.3*markersize );
          cr->set_font_size( 3*markersize );
          cr->save();
          cr->set_source_rgb( 1, 1, 1 );
          cr->paint();
          cr->restore();
          draw.set_markersize(markersize);
          draw.draw(cr);
          // calculate left bottom corner in TASCAR coordinate system:
          TASCAR::pos_t p_o(-0.4*width,-0.4*height,0);
          p_o /= wscale;
          p_o = draw.view.inverse(p_o);
          TASCAR::pos_t p_x(0.1*draw.view.scale,0,0);
          TASCAR::pos_t p_y(0,0.1*draw.view.scale,0);
          TASCAR::pos_t p_z(0,0,0.1*draw.view.scale);
          p_x += p_o;
          p_y += p_o;
          p_z += p_o;
          p_o = draw.view(p_o);
          p_x = draw.view(p_x);
          p_y = draw.view(p_y);
          p_z = draw.view(p_z);
          cr->set_source_rgb(1,0,0);
          draw.draw_edge(cr,p_o,p_x);
          cr->stroke();
          cr->set_source_rgb(0,1,0);
          draw.draw_edge(cr,p_o,p_y);
          cr->stroke();
          cr->set_source_rgb(0,0,1);
          draw.draw_edge(cr,p_o,p_z);
          cr->stroke();
          p_x = TASCAR::pos_t(10.0*draw.view.scale,0,0);
          p_y = TASCAR::pos_t(0,10.0*draw.view.scale,0);
          p_z = TASCAR::pos_t(0,0,10.0*draw.view.scale);
          TASCAR::pos_t p_x1(p_x);
          TASCAR::pos_t p_y1(p_y);
          TASCAR::pos_t p_z1(p_z);
          p_x1*=-1.0;
          p_y1*=-1.0;
          p_z1*=-1.0;
          p_x = draw.view(p_x);
          p_y = draw.view(p_y);
          p_z = draw.view(p_z);
          p_x1 = draw.view(p_x1);
          p_y1 = draw.view(p_y1);
          p_z1 = draw.view(p_z1);
          cr->set_source_rgba(1,0,0,0.17);
          draw.draw_edge(cr,p_x1,p_x);
          cr->stroke();
          cr->set_source_rgba(0,1,0,0.17);
          draw.draw_edge(cr,p_y1,p_y);
          cr->stroke();
          cr->set_source_rgba(0,0,1,0.17);
          draw.draw_edge(cr,p_z1,p_z);
          cr->stroke();
          pthread_mutex_unlock( &mtx_draw );
        }
      }
      session->unlock_vars();
    }
  }
  return true;
}

void tascar_window_t::load(const std::string& fname)
{
  warnings.clear();
  scene_load(fname);
  tascar_filename = fname;
  sessionquit = false;
  if( session ){
    session->add_bool_true("/tascargui/quit", &sessionquit );
  }
  reset_gui();
}

void tascar_window_t::on_active_track_changed()
{
  if( session && (session->scenes.size() > selected_scene) ){
    if( active_track->get_active() ){
      if( active_object )
        session->scenes[selected_scene]->guitrackobject = active_object;
    }else{
      session->scenes[selected_scene]->guitrackobject = NULL;
    }
  }
}

void tascar_window_t::on_active_selector_changed()
{
  if( session && (session->scenes.size() > selected_scene) ){
    // do something.
    std::string sc(active_selector->get_active_text());
    if( sc == "(none)" )
      active_object = NULL;
    else{
      std::vector<TASCAR::Scene::object_t*> match(session->scenes[selected_scene]->find_object(sc));
      if( match.size() )
        active_object = match[0];
      else
        active_object = NULL;
    }
    update_selection_info();
  }
  draw.select_object( active_object );
}  

void tascar_window_t::on_scene_selector_changed()
{
  if( session ){
    std::string sc(scene_selector->get_active_text());
    for(unsigned int k=0;k<session->scenes.size();k++)
      if( session->scenes[k]->name == sc )
        selected_scene = k;
  }else{
    selected_scene = 0;
  }
  if( session && (session->scenes.size() > selected_scene) ){
    draw.set_scene( session->scenes[selected_scene] );
    source_panel->set_scene( session->scenes[selected_scene], session );
    draw.view.set_scale( session->scenes[selected_scene]->guiscale );
    // fill object list:
    update_levelmeter_settings();
    update_object_list();
  }else{
    draw.set_scene( NULL );
    source_panel->set_scene( NULL, NULL );
    draw.view.set_scale(20);
  }
}  

void tascar_window_t::update_selection_info()
{
  active_mixer->remove();
  if( active_source_ctl )
    delete active_source_ctl;
  active_source_ctl = NULL;
  if( session && active_object ){
    active_type_label->set_text(active_object->get_type());
    active_track->set_active(session->scenes[selected_scene]->guitrackobject==active_object);
    char ctmp[1024];
    xmlpp::Element* e(active_object->TASCAR::dynobject_t::e);
    sprintf(ctmp,"Line %d:",e->get_line());
    active_label_sourceline->set_text(ctmp);
    xmlpp::Document doc;
    doc.create_root_node_by_import(e);
    std::string disp(doc.write_to_string_formatted());
    // remove first line:
    size_t p(disp.find(10));
    if( p < disp.npos )
      ++p;
    disp.erase(disp.begin(),disp.begin()+p);
    active_source_display->get_buffer()->set_text(disp);
    active_source_ctl = new source_ctl_t(session->scenes[selected_scene],active_object);
    active_mixer->add(*active_source_ctl);
    active_mixer->show_all();
    active_source_ctl->set_levelmeter_mode(source_panel->lmode);
    active_source_ctl->set_levelmeter_range(session->levelmeter_min,session->levelmeter_range);
  }else{
    active_type_label->set_text("");
    active_track->set_active(false);
    active_label_sourceline->set_text("");
    active_source_display->get_buffer()->set_text("");
  }
}

void tascar_window_t::update_object_list()
{
  active_selector->remove_all();
  active_selector->append("(none)");
  active_selector->set_active(0);
  active_object = NULL;
  draw.select_object(NULL);
  if( session && (session->scenes.size() > selected_scene) ){
    for( std::vector<TASCAR::Scene::object_t*>::const_iterator it=session->scenes[selected_scene]->all_objects.begin();
         it!=session->scenes[selected_scene]->all_objects.end(); ++it)
      active_selector->append((*it)->get_name());
  }
  update_selection_info();
}

void tascar_window_t::update_levelmeter_settings()
{
  if( session  && source_panel ){
    source_panel->set_levelmeter_mode(session->levelmeter_mode);
    source_panel->set_levelmeter_range(session->levelmeter_min,session->levelmeter_range);
    if( active_source_ctl ){
      active_source_ctl->set_levelmeter_mode(source_panel->lmode);
      active_source_ctl->set_levelmeter_range(session->levelmeter_min,session->levelmeter_range);
    }
  }
}

void tascar_window_t::reset_gui()
{
  timeline->clear_marks();
  scene_selector->remove_all();
  scene_selector->set_active(0);
  selected_range = -1;
  if( session ){
    int32_t mainwin_width(1600);
    int32_t mainwin_height(900);
    if( session->scenes.empty() ){
      mainwin_width = 200;
      mainwin_height = 60;
    }
    int32_t mainwin_x(0);
    int32_t mainwin_y(0);
    get_position( mainwin_x, mainwin_y );
    TASCAR::xml_element_t mainwin(session->tsc_reader_t::find_or_add_child("mainwindow"));
    mainwin.get_attribute( "w", mainwin_width );
    mainwin.get_attribute( "h", mainwin_height );
    mainwin.get_attribute( "x", mainwin_x );
    mainwin.get_attribute( "y", mainwin_y );
    resize( mainwin_width, mainwin_height );
    move( mainwin_x, mainwin_y );
    resize( mainwin_width, mainwin_height );
    move( mainwin_x, mainwin_y );
    timeline->set_range(0,session->duration);
    for(unsigned int k=0;k<session->ranges.size();k++){
      timeline->add_mark(session->ranges[k]->start,Gtk::POS_BOTTOM,"");
      timeline->add_mark(session->ranges[k]->end,Gtk::POS_BOTTOM,"");
    }
    for( std::vector<TASCAR::scene_render_rt_t*>::iterator it=session->scenes.begin();it!=session->scenes.end();++it)
      scene_selector->append((*it)->name);
    selected_scene = 0;
    scene_selector->set_active(0);
    if( session->scenes.size() > selected_scene )
      set_scale(session->scenes[selected_scene]->guiscale);
  }
  if( session ){
    set_title("tascar - " + session->name + " [" + basename(tascar_filename.c_str()) + "]");
  }else{
    set_title("tascar");
    resize( 200, 60 );
  }
  update_object_list();
  if( session && (session->scenes.size() > selected_scene) ){
    draw.set_scene( session->scenes[selected_scene] );
    source_panel->set_scene( session->scenes[selected_scene], session );
    draw.view.set_scale( session->scenes[selected_scene]->guiscale );
    update_levelmeter_settings();
  }
  if( session ){
    source_buffer->set_text(session->doc->write_to_string_formatted());
    osc_vars->get_buffer()->set_text(session->list_variables());
    text_srv_addr->set_text(session->osc_srv_addr);
    text_srv_port->set_text(session->osc_srv_port);
    legal_view->get_buffer()->set_text(session->legal_stuff());
    //legal_view->get_buffer()->set_text("");
    //legal_view->get_buffer()->insert_markup(legal_view->get_buffer()->end(),session->legal_stuff(true));
  }else{
    source_view.get_source_buffer()->set_text("");
    osc_vars->get_buffer()->set_text("");
    text_srv_addr->set_text("");
    text_srv_port->set_text("");
    legal_view->get_buffer()->set_text("");
  }
  on_menu_view_show_warnings();
}

void tascar_window_t::on_menu_file_quit()
{
  hide(); //Closes the main window to stop the app->run().
}

void tascar_window_t::on_menu_file_close()
{
  try{
    scene_destroy();
    warnings.clear();
  }
  catch( const std::exception& e){
    error_message(e.what());
  }
  reset_gui();
}

void tascar_window_t::on_menu_file_new()
{
  try{
    scene_new();
  }
  catch( const std::exception& e){
    error_message(e.what());
  }
  reset_gui();
}

void tascar_window_t::on_menu_file_exportcsv()
{
  if( session ){
    Gtk::FileChooserDialog dialog("Please choose a destination",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    //Add response buttons the the dialog:
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
    //Add filters, so that only certain file types can be selected:
    Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
    filter_tascar->set_name("CSV files");
    filter_tascar->add_pattern("*.csv");
    dialog.add_filter(filter_tascar);
    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);
    //Show the dialog and wait for a user response:
    int result = dialog.run();
    //Handle the response:
    if( result == Gtk::RESPONSE_OK){
      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      if( filename.find(".csv") == std::string::npos )
        filename += ".csv";
      if( filename.size() > 0 ){
        try{
          std::ofstream ofs(filename.c_str());
          ofs << "\"Name\",\"PosX\",\"PosY\",\"PosZ\",\"EulerZ\",\"EulerY\",\"EulerX\"\n";
          for(uint32_t kscene=0;kscene<session->scenes.size();kscene++){
            ofs << "\"" << session->scenes[kscene]->name << "\", , , , , , \n";
            std::vector<TASCAR::Scene::object_t*> obj(session->scenes[kscene]->get_objects());
            for(uint32_t k=0;k<obj.size();k++){
              TASCAR::pos_t p;
              TASCAR::zyx_euler_t o;
              obj[k]->get_6dof(p,o);
              ofs << "\"" << obj[k]->get_name() << "\"," << 
                p.x << "," <<
                p.y << "," <<
                p.z << "," <<
                o.z*RAD2DEG << "," <<
                o.y*RAD2DEG << "," <<
                o.x*RAD2DEG << "\n";
            }
          }
        }
        catch( const std::exception& e){
          error_message(e.what());
        }
      }
    }
  }
}

void tascar_window_t::on_menu_file_exportcsvsounds()
{
  if( session ){
    Gtk::FileChooserDialog dialog("Please choose a destination",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    //Add response buttons the the dialog:
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
    //Add filters, so that only certain file types can be selected:
    Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
    filter_tascar->set_name("CSV files");
    filter_tascar->add_pattern("*.csv");
    dialog.add_filter(filter_tascar);
    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);
    //Show the dialog and wait for a user response:
    int result = dialog.run();
    //Handle the response:
    if( result == Gtk::RESPONSE_OK){
      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      if( filename.find(".csv") == std::string::npos )
        filename += ".csv";
      if( filename.size() > 0 ){
        try{
          std::ofstream ofs(filename.c_str());
          ofs << "\"Name\",\"PosX\",\"PosY\",\"PosZ\"\n";
          for(uint32_t kscene=0;kscene<session->scenes.size();kscene++){
            ofs << "\"" << session->scenes[kscene]->name << "\", , , \n";
            //std::vector<TASCAR::Scene::sound_t*> obj(session->scenes[kscene]->linearize_sounds());
            for(std::vector<TASCAR::Scene::sound_t*>::iterator iam=session->scenes[kscene]->sounds.begin();iam != session->scenes[kscene]->sounds.end();++iam){
              TASCAR::pos_t p((*iam)->position);
              ofs << "\"" << (*iam)->get_fullname() << "\"," << 
                p.x << "," <<
                p.y << "," <<
                p.z << "\n";
            }
          }
        }
        catch( const std::exception& e){
          error_message(e.what());
        }
      }
    }
  }
}

void tascar_window_t::on_menu_file_exportpdf()
{
  if( session ){
    Gtk::FileChooserDialog dialog("Please choose a destination",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    //Add response buttons the the dialog:
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
    //Add filters, so that only certain file types can be selected:
    Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
    filter_tascar->set_name("PDF files");
    filter_tascar->add_pattern("*.pdf");
    dialog.add_filter(filter_tascar);
    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);
    //Show the dialog and wait for a user response:
    int result = dialog.run();
    //Handle the response:
    if( result == Gtk::RESPONSE_OK){
      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      if( filename.find(".pdf") == std::string::npos )
        filename += ".pdf";
      if( filename.size() > 0 ){
        try{
          TASCAR::pdfexport_t pdfexport(session,filename,false);
        }
        catch( const std::exception& e){
          error_message(e.what());
        }
      }
    }
  }
}

void tascar_window_t::on_menu_file_exportacmodel()
{
  if( session ){
    Gtk::FileChooserDialog dialog("Please choose a destination",
                                  Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    //Add response buttons the the dialog:
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
    //Add filters, so that only certain file types can be selected:
    Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
    filter_tascar->set_name("PDF files");
    filter_tascar->add_pattern("*.pdf");
    dialog.add_filter(filter_tascar);
    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);
    //Show the dialog and wait for a user response:
    int result = dialog.run();
    //Handle the response:
    if( result == Gtk::RESPONSE_OK){
      //Notice that this is a std::string, not a Glib::ustring.
      std::string filename = dialog.get_filename();
      if( filename.find(".pdf") == std::string::npos )
        filename += ".pdf";
      if( filename.size() > 0 ){
        try{
          TASCAR::pdfexport_t pdfexport(session,filename,true);
        }
        catch( const std::exception& e){
          error_message(e.what());
        }
      }
    }
  }
}

void tascar_window_t::on_menu_file_reload()
{
  try{
    warnings.clear();
    scene_load(tascar_filename);
    sessionquit = false;
    if( session )
      session->add_bool_true("/tascargui/quit", &sessionquit );
    reset_gui();
  }
  catch( const std::exception& e){
    error_message(e.what());
  }
}

void tascar_window_t::on_menu_file_open()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
  //Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  //Add filters, so that only certain file types can be selected:
  Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
  filter_tascar->set_name("tascar files");
  filter_tascar->add_pattern("*.tsc");
  dialog.add_filter(filter_tascar);
  Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any files");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);
  //Show the dialog and wait for a user response:
  int result = dialog.run();
  //Handle the response:
  if( result == Gtk::RESPONSE_OK){
    //Notice that this is a std::string, not a Glib::ustring.
    std::string filename = dialog.get_filename();
    try{
      warnings.clear();
      scene_load(filename);
      tascar_filename = filename;
      sessionquit = false;
      if( session )
        session->add_bool_true("/tascargui/quit", &sessionquit );
      reset_gui();
    }
    catch( const std::exception& e){
      error_message(e.what());
    }
  }
}

void tascar_window_t::on_menu_file_open_example()
{
  Gtk::FileChooserDialog dialog("Please choose a file",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_current_folder("/usr/share/tascar/examples/");
  dialog.set_transient_for(*this);
  //Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
  //Add filters, so that only certain file types can be selected:
  Glib::RefPtr<Gtk::FileFilter> filter_tascar = Gtk::FileFilter::create();
  filter_tascar->set_name("tascar files");
  filter_tascar->add_pattern("*.tsc");
  dialog.add_filter(filter_tascar);
  Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
  filter_any->set_name("Any files");
  filter_any->add_pattern("*");
  dialog.add_filter(filter_any);
  //Show the dialog and wait for a user response:
  int result = dialog.run();
  //Handle the response:
  if( result == Gtk::RESPONSE_OK){
    //Notice that this is a std::string, not a Glib::ustring.
    std::string filename = dialog.get_filename();
    try{
      warnings.clear();
      scene_load(filename);
      tascar_filename = filename;
      sessionquit = false;
      if( session )
        session->add_bool_true("/tascargui/quit", &sessionquit );
      reset_gui();
    }
    catch( const std::exception& e){
      error_message(e.what());
    }
  }
}

void tascar_window_t::on_menu_view_meter_rmspeak()
{
  if( session )
    session->levelmeter_mode = "rmspeak";
  update_levelmeter_settings();
}

void tascar_window_t::on_menu_view_meter_rms()
{
  if( session )
    session->levelmeter_mode = "rms";
  update_levelmeter_settings();
}

void tascar_window_t::on_menu_view_meter_peak()
{
  if( session )
    session->levelmeter_mode = "peak";
  update_levelmeter_settings();
}

void tascar_window_t::on_menu_view_meter_percentile()
{
  if( session )
    session->levelmeter_mode = "percentile";
  update_levelmeter_settings();
}

void tascar_window_t::on_menu_view_viewport_xy()
{
  draw.set_viewport( scene_draw_t::xy );
}

void tascar_window_t::on_menu_view_viewport_rotz()
{
  draw.view.euler.z += M_PI/24.0;
}

void tascar_window_t::on_menu_view_viewport_rotzcw()
{
  draw.view.euler.z -= M_PI/24.0;
}

void tascar_window_t::on_menu_view_viewport_setref()
{
  if( session && (session->scenes.size() > selected_scene) )
      session->scenes[selected_scene]->guitrackobject = NULL;
  draw.select_object(NULL);
  draw.view.ref = TASCAR::pos_t();
  active_track->set_active(false);
}

void tascar_window_t::on_menu_view_viewport_xyz()
{
  draw.set_viewport( scene_draw_t::xyz );
}

void tascar_window_t::on_menu_view_viewport_xz()
{
  draw.set_viewport( scene_draw_t::xz );
}

void tascar_window_t::on_menu_view_viewport_yz()
{
  draw.set_viewport( scene_draw_t::yz );
}

void tascar_window_t::on_menu_view_show_warnings()
{
  std::string v;
  for(std::vector<std::string>::const_iterator it=warnings.begin();it!=warnings.end();++it){
    v+= "Warning: " + *it + "\n";
  }
  if( session ){
    v+= session->show_unknown();
    session->validate_attributes(v);
  }
  text_warnings->get_buffer()->set_text(v);
  if( !v.empty() )
    notebook->set_current_page(5);
}

void tascar_window_t::on_menu_view_zoom_in()
{
  if( session && (session->scenes.size() > selected_scene) ){
    draw.view.set_scale(session->scenes[selected_scene]->guiscale /= 1.1892071150027210269);
  }
}

void tascar_window_t::on_menu_view_zoom_out()
{
  if( session && (session->scenes.size() > selected_scene) ){
    draw.view.set_scale(session->scenes[selected_scene]->guiscale *= 1.1892071150027210269);
  }
}

bool tascar_window_t::on_map_clicked(GdkEventButton* e)
{
  if( session && (session->scenes.size() > selected_scene) ){
    if( e->type == GDK_BUTTON_PRESS ){
      Gtk::Allocation allocation = scene_map->get_allocation();
      const int width = allocation.get_width();
      const int height = allocation.get_height();
      TASCAR::pos_t mousepos(e->x-0.5*width,-(e->y-0.5*height),0);
      double wscale(0.5*std::max(height,width));
      mousepos*=1.0/wscale;
      if( pthread_mutex_trylock( &mtx_draw ) == 0 ){
        double dmin(DBL_MAX);
        TASCAR::Scene::object_t* obj_nearest(NULL);
        for( std::vector<TASCAR::Scene::object_t*>::iterator it=session->scenes[selected_scene]->all_objects.begin();
             it!=session->scenes[selected_scene]->all_objects.end();++it){
          TASCAR::pos_t opos(draw.view((*it)->c6dof.position));
          opos.z = 0;
          double d(distance(mousepos,opos));
          if( (d < dmin) && (d*wscale < 30) ){
            dmin = d;
            obj_nearest = *it;
          }
        }
        if( obj_nearest ){
          //active selection
          active_object = obj_nearest;
          active_selector->set_active_text(active_object->get_name());
          update_selection_info();
          draw.select_object( active_object );
        }
        pthread_mutex_unlock( &mtx_draw );
      }
    }
  }
  return false;
}

bool tascar_window_t::on_map_scroll(GdkEventScroll * e)
{
  if( ((e->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0) && session ){
    switch( e->direction ){
    case GDK_SCROLL_UP :
      on_menu_view_zoom_in();
      break;
    case GDK_SCROLL_DOWN :
      on_menu_view_zoom_out();
      break;
    case GDK_SCROLL_LEFT :
    case GDK_SCROLL_RIGHT :
    case GDK_SCROLL_SMOOTH :
      break;
    }
  }
  return true;
}

void tascar_window_t::on_time_changed()
{
  double ltime(timeline->get_value());
  if( session ){
    if( ltime != draw.get_time() ){
      session->tp_locate(ltime);
    }
  }
}

void tascar_window_t::on_menu_help_manual()
{
  // open tascar manual
  // this is deprecated in Gtk 3.3:
  gtk_show_uri( NULL, "file:///usr/share/doc/tascar/manual.pdf", GDK_CURRENT_TIME, NULL );
  //Gtk::gtk_show_uri_on_window( this, "file:///usr/share/doc/tascar/manual.pdf", GDK_CURRENT_TIME, NULL );
}

void tascar_window_t::on_menu_help_bugreport()
{
  // open tascar manual
  // this is deprecated in Gtk 3.3:

  std::string uri("https://github.com/gisogrimm/tascar/issues/new?body=TASCARpro%20version%20");
  uri += TASCARVER;
  gtk_show_uri( NULL, uri.c_str(),
                GDK_CURRENT_TIME, NULL );
  //Gtk::gtk_show_uri_on_window( this, "file:///usr/share/doc/tascar/manual.pdf", GDK_CURRENT_TIME, NULL );
}

void tascar_window_t::on_menu_help_about()
{
  Gtk::AboutDialog* aboutdialog(NULL);
  m_refBuilder->get_widget("aboutdialog",aboutdialog);
  if( aboutdialog ){
    aboutdialog->run();
    aboutdialog->hide();
  }
}

void tascar_window_t::on_menu_transport_play()
{
  if( session ){
    session->tp_start();
  }
}

void tascar_window_t::on_menu_transport_stop()
{
  if( session ){
    session->tp_stop();
  }
}

void tascar_window_t::on_menu_transport_rewind()
{
  if( session ){
    double cur_time(std::max(0.0,session->tp_get_time()-10.0));
    session->tp_locate(cur_time);
  }
}

void tascar_window_t::on_menu_transport_forward()
{
  if( session ){
    double cur_time(std::min(session->duration,session->tp_get_time()+10.0));
    session->tp_locate(cur_time);
  }
}

void tascar_window_t::on_menu_transport_previous()
{
  if( session ){
    session->tp_locate(0.0);
  }
}

void tascar_window_t::on_menu_transport_next()
{
  if( session ){
    session->tp_locate(session->duration);
  }
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

