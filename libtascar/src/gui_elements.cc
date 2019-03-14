#include "gui_elements.h"

#define GUI_FACE_ALPHA 0.1

dameter_t::dameter_t()
  : v_rms(0),
    v_peak(0),
    q30(0),
    q50(0),
    q65(0),
    q95(0),
    q99(0),
    vmin(30),
    range(70),
    targetlevel(0)
{
}

void dameter_t::invalidate_win()
{
  queue_draw();
}

void line_at(const Cairo::RefPtr<Cairo::Context>& cr, float y, float x0, float x1)
{
  cr->move_to(x0,y);
  cr->line_to(x1,y);
  cr->stroke();
}

bool dameter_t::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  Gtk::Allocation allocation(get_allocation());
  const int width(allocation.get_width());
  const int height(allocation.get_height());
  const int label_h(20);
  cr->save();
  float yscale((height-label_h)/range);
  cr->translate(0,height);
  cr->scale( 1.0, -yscale );
  cr->translate(0,-vmin);
  switch( mode ){
  case rmspeak:
  case rms:
  case peak:
    cr->set_source_rgb( 0, 0, 0 );
    break;
  case percentile:
    cr->set_source_rgb( 0.8, 0.8, 0.8 );
    break;
  }
  cr->paint();
  switch( mode ){
  case rmspeak:
  case rms:
    cr->set_source_rgb( 0.25, 0.7, 0.25 );
    cr->rectangle(0,0,0.5*width,v_rms);
    cr->fill();
    break;
  case peak:
    break;
  case percentile:
    cr->set_source_rgb( 0.6, 0.6, 0.6 );
    cr->rectangle(0,0,0.5*width,q30);
    cr->fill();
    cr->set_source_rgb( 0.0, 1.0, 119.0/255.0 );
    cr->rectangle(0,q30,0.5*width,q50-q30);
    cr->fill();
    cr->set_source_rgb( 0.0, 187.0/255.0, 153.0/255.0 );
    cr->rectangle(0,q50,0.5*width,q65-q50);
    cr->fill();
    cr->set_source_rgb( 0.0, 119.0/255.0, 187.0/255.0 );
    cr->rectangle(0,q65,0.5*width,q95-q65);
    cr->fill();
    cr->set_source_rgb( 0.0, 51.0/255.0, 221.0/255.0 );
    cr->rectangle(0,q95,0.5*width,q99-q95);
    cr->fill();
    cr->set_line_width(2/yscale);
    cr->set_source_rgb( 0.3, 0.3, 0.3 );
    line_at(cr,q30,0,0.5*width);
    cr->set_source_rgb( 0.0, 0.5, 0.5*119.0/255.0 );
    line_at(cr,q50,0,0.5*width);
    cr->set_source_rgb( 0.0, 0.5*187.0/255.0, 0.5*153.0/255.0 );
    line_at(cr,q65,0,0.5*width);
    cr->set_source_rgb( 0.0, 0.5*119.0/255.0, 0.5*187.0/255.0 );
    line_at(cr,q95,0,0.5*width);
    cr->set_source_rgb( 0.0, 0.5*51.0/255.0, 0.5*221.0/255.0 );
    line_at(cr,q99,0,0.5*width);
    break;
  }
  switch( mode ){
  case rmspeak:
  case peak:
    cr->set_line_width(1.0);
    cr->set_source_rgb( 1.0, 0.9, 0 );
    cr->move_to(0,v_peak);
    cr->line_to(0.5*width,v_peak);
    cr->stroke();
    break;
  default:
    break;
  }
  // draw scale:
  cr->set_line_width(1/yscale);
  float divider(10.0f);
  if( range < 50.0f )
    divider = 5.0f;
  if( range < 30.0f )
    divider = 2.0f;
  for(int32_t k=ceil((vmin+1e-7)/divider);k<=floor((vmin+range-1e-7)/divider);++k){
    switch( mode ){
    case rmspeak:
    case rms:
    case peak:
      cr->set_source_rgb( 0.25, 0.7, 0.25 );
      break;
    case percentile:
      cr->set_source_rgb( 0.4, 0.4, 0.4 );
      break;
    }
    line_at(cr,divider*k,0.5*width,width);
    cr->save();
    cr->move_to(0.52*width,divider*k+1/yscale);
    cr->scale(1.0,-1.0/yscale);
    char ctmp[256];
    sprintf(ctmp,"%1.0f",divider*k);
    switch( mode ){
    case rmspeak:
    case rms:
    case peak:
      cr->set_source_rgb( 0.45, 1, 0.35 );
      break;
    case percentile:
      cr->set_source_rgb( 0, 0, 0 );
      break;
    }
    cr->show_text(ctmp);
    cr->restore();
  }
  cr->set_line_width(1);
  cr->set_source_rgba( 1, 0, 0, 0.7 );
  line_at(cr,targetlevel,0.5*width,width);
  cr->restore();
  // print level:
  cr->save();
  cr->set_source_rgb( 1, 1, 1 );
  cr->rectangle(0,0,width,20);
  cr->fill();
  {
    char ctmp[256];
    sprintf(ctmp,"%1.1f",v_rms);
    cr->set_source_rgb( 0, 0, 0 );
    cr->move_to( 0.1*width, 0.8*label_h );
    cr->scale( 1.2, 1.2 );
    cr->show_text(ctmp);
  }
  cr->restore();
  return true;
}

playertimeline_t::playertimeline_t()
{
}

splmeter_t::splmeter_t()
{
  dameter.signal_draw().connect( sigc::mem_fun(*this,&splmeter_t::on_draw) );
  add(dameter);
  dameter.set_size_request( 32, 320 );
}

void splmeter_t::invalidate_win()
{
  dameter.invalidate_win();
}

void splmeter_t::set_mode( dameter_t::mode_t mode )
{
  dameter.mode = mode;
}

void splmeter_t::set_min_and_range( float vmin, float range )
{
  dameter.vmin = vmin;
  dameter.range = range;
}

void splmeter_t::update_levelmeter( const TASCAR::levelmeter_t& lm, float targetlevel )
{
  dameter.targetlevel = targetlevel;
  lm.get_rms_and_peak( dameter.v_rms, dameter.v_peak );
  //char ctmp[256];
  switch( dameter.mode ){
  case dameter_t::rmspeak:
  case dameter_t::rms:
  case dameter_t::peak:
    //sprintf(ctmp,"%1.1f",dameter.v_rms);
    //val.set_text(ctmp);
    break;
  case dameter_t::percentile:
    lm.get_percentile_levels( dameter.q30, dameter.q50, dameter.q65, dameter.q95, dameter.q99 );
    //sprintf(ctmp,"%1.1f",dameter.q65);
    //val.set_text(ctmp);
    break;
  }
}

GainScale_t::GainScale_t()
  : Gtk::Scale(Gtk::ORIENTATION_VERTICAL),
    ap_(NULL),
    vmin(-30),
    vmax(10)
{
  set_draw_value(false);
  set_has_origin(true);
  set_range(vmin,vmax);
  set_inverted(true);
  set_increments(1,1);
}

void GainScale_t::on_value_changed()
{
  if( ap_ )
    ap_->set_gain_db( get_value() );
}

void GainScale_t::set_inv( bool inv)
{
  if( ap_ ){
    ap_->set_inv(inv);
  }
}

float GainScale_t::update(bool& inv)
{
  double v(0);
  if( ap_ ){
    inv = ap_->get_inv();
    v = ap_->get_gain_db();
    if( (v < vmin) || (v > vmax) ){
      vmin = std::min(vmin,v);
      vmax = std::max(vmax,v);
      set_range(vmin,vmax);
    }
    set_value(v);
  }
  return v;
}

void GainScale_t::set_src(TASCAR::Scene::audio_port_t* ap)
{
  ap_ = ap;
}

gainctl_t::gainctl_t()
{
  val.set_has_frame( false );
  val.set_max_length( 10 );
  val.set_width_chars( 4 );
  val.set_size_request( 32, -1 );
  scale.set_size_request( -1, 280 );
  polarity.set_label("ø");
  add(box);
  box.pack_start(polarity,Gtk::PACK_SHRINK);
  box.pack_start(val,Gtk::PACK_SHRINK);
  box.add(scale);
  scale.signal_value_changed().connect(sigc::mem_fun(*this,&gainctl_t::on_scale_changed));
  val.signal_activate().connect(sigc::mem_fun(*this,&gainctl_t::on_text_changed));
  polarity.signal_toggled().connect(sigc::mem_fun(*this,&gainctl_t::on_inv_changed));
}

void gainctl_t::update()
{
  bool inv(false);
  scale.update(inv);
  polarity.set_active(inv);
}

void gainctl_t::set_src(TASCAR::Scene::audio_port_t* ap)
{
  scale.set_src(ap);
  on_scale_changed();
}

void gainctl_t::on_scale_changed()
{
  char ctmp[256];
  float v(scale.get_value());
  sprintf(ctmp,"%1.1f",v);
  val.set_text(ctmp);
}

void gainctl_t::on_inv_changed()
{
  bool inv(polarity.get_active());
  scale.set_inv(inv);
}

void gainctl_t::on_text_changed()
{
  std::string txt(val.get_text());
  char* endp(NULL);
  float v(0);
  v = strtof(txt.c_str(),&endp);
  if( endp != txt.c_str() ){
    scale.set_value(v);
  }
}

bool scene_draw_t::draw_edge(Cairo::RefPtr<Cairo::Context> cr, pos_t p1, pos_t p2)
{
  if( !(p1.has_infinity() || p2.has_infinity()) ){
    cr->move_to(p1.x,-p1.y);
    cr->line_to(p2.x,-p2.y);
    return true;
  }
  return false;
}

source_ctl_t::~source_ctl_t()
{
  for(uint32_t k=0;k<meters.size();k++)
    delete meters[k];
  for(uint32_t k=0;k<gainctl.size();k++)
    delete gainctl[k];
}

source_ctl_t::source_ctl_t(lo_address client_addr, TASCAR::Scene::scene_t* s, TASCAR::Scene::route_t* r)
  : mute("M"),solo("S"),client_addr_(client_addr),name_(r->get_name()),scene_(s),route_(r),use_osc(true)
{
  setup();
}

source_ctl_t::source_ctl_t(TASCAR::Scene::scene_t* s, TASCAR::Scene::route_t* r)
  : mute("M"),solo("S"),name_(r->get_name()),scene_(s),route_(r),use_osc(false)
{
  setup();
}

void source_ctl_t::setup()
{
  ebox.add( box );
  label.set_text(route_->get_name());
  if( dynamic_cast<TASCAR::Scene::face_object_t*>(route_))
    tlabel.set_text("face");
  else if( dynamic_cast<TASCAR::Scene::face_group_t*>(route_))
    tlabel.set_text("fgrp");
  else if( dynamic_cast<TASCAR::Scene::obstacle_group_t*>(route_))
    tlabel.set_text("obstacle");
  else if( dynamic_cast<TASCAR::Scene::src_object_t*>(route_))
    tlabel.set_text("src");
  else if( dynamic_cast<TASCAR::Scene::diffuse_info_t*>(route_))
    tlabel.set_text("dif");
  else if( dynamic_cast<TASCAR::Scene::receivermod_object_t*>(route_))
    tlabel.set_text("rcvr");
  else 
    tlabel.set_text("route");
  TASCAR::Scene::audio_port_t* ap(dynamic_cast<TASCAR::Scene::audio_port_t*>(route_));
  if( ap ){
    gainctl.push_back(new gainctl_t());
    gainctl.back()->set_src( ap );
    meterbox.add(*(gainctl.back()));
  }else{
    TASCAR::Scene::src_object_t* so(dynamic_cast<TASCAR::Scene::src_object_t*>(route_));
    if( so ){
      for( uint32_t k=0;k<so->sound.size();++k){
        gainctl.push_back(new gainctl_t());
        gainctl.back()->set_src( so->sound[k] );
        meterbox.add(*(gainctl.back()));
      }
    }
  }
  box.pack_start( tlabel, Gtk::PACK_SHRINK );
  box.pack_start( label, Gtk::PACK_SHRINK );//EXPAND_PADDING );
  msbox.pack_start( mute, Gtk::PACK_EXPAND_WIDGET );
  msbox.pack_start( solo, Gtk::PACK_EXPAND_WIDGET );
  box.pack_start( msbox, Gtk::PACK_SHRINK );
  mute.set_active(route_->get_mute());
  solo.set_active(route_->get_solo());
#ifdef GTKMM30
  Gdk::RGBA col_yellow("f4e83a");
  col_yellow.set_rgba(244.0/256,232.0/256,58.0/256,0.5);
  Gdk::RGBA col;
  if( TASCAR::Scene::object_t* o=dynamic_cast<TASCAR::Scene::object_t*>(route_) ){
    TASCAR::Scene::rgb_color_t c(o->color);
    col.set_rgba(c.r,c.g,c.b,0.3);
    ebox.override_background_color(col);
  }
#else
  Gdk::Color col;
  col.set_rgb(244*256,232*256,58*256);
  mute.modify_bg(Gtk::STATE_ACTIVE,col);
  mute.modify_bg(Gtk::STATE_PRELIGHT,col);
  mute.modify_bg(Gtk::STATE_SELECTED,col);
  col.set_rgb(244*256,30*256,30*256);
  solo.modify_bg(Gtk::STATE_ACTIVE,col);
  solo.modify_bg(Gtk::STATE_PRELIGHT,col);
  if( object_t* o=dynamic_cast<object_t*>(r) ){
    rgb_color_t c(o->color);
    col.set_rgb_p(0.5+0.3*c.r,0.5+0.3*c.g,0.5+0.3*c.b);
    ebox.modify_bg(Gtk::STATE_NORMAL,col);
  }
#endif
  frame.add(ebox);
  pack_start( frame, Gtk::PACK_SHRINK );
  pack_start( meterbox, Gtk::PACK_SHRINK );
  pack_start( playertimeline, Gtk::PACK_EXPAND_WIDGET );
  for(uint32_t k=0;k<route_->metercnt();k++){
    meters.push_back(new splmeter_t());
    meterbox.add(*(meters[k]));
  }
  mute.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_mute));
  solo.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_solo));
}

void source_ctl_t::update()
{
  // update level meters:
  for(uint32_t k=0;k<meters.size();k++)
    meters[k]->update_levelmeter( route_->get_meter(k), route_->targetlevel );
  // update gain controllers:
  for(uint32_t k=0;k<gainctl.size();k++)
    gainctl[k]->update();
  // update mute/solo controls:
  mute.set_active(route_->get_mute());
  solo.set_active(route_->get_solo());
}

void source_ctl_t::invalidate_win()
{
  for(uint32_t k=0;k<meters.size();k++)
    meters[k]->invalidate_win();
}

void source_ctl_t::on_mute()
{
  bool m(mute.get_active());
  if( use_osc ){
    std::string path("/"+scene_->name+"/"+name_+"/mute");
    lo_send(client_addr_,path.c_str(),"i",m);
  }else{
    route_->set_mute(m);
  }
}

void source_ctl_t::set_levelmeter_mode( dameter_t::mode_t mode )
{
  for(std::vector<splmeter_t*>::iterator it=meters.begin();it!=meters.end();++it)
    (*it)->set_mode( mode );
}


void source_ctl_t::set_levelmeter_range( float vmin, float range )
{
  for(std::vector<splmeter_t*>::iterator it=meters.begin();it!=meters.end();++it)
    (*it)->set_min_and_range( vmin, range );
}

void source_ctl_t::on_solo()
{
  bool m(solo.get_active());
  if( use_osc ){
    std::string path("/"+scene_->name+"/"+name_+"/solo");
    lo_send(client_addr_,path.c_str(),"i",m);
  }else{
    route_->set_solo(m,scene_->anysolo);
  }
}

source_panel_t::source_panel_t(lo_address client_addr)
  : client_addr_(client_addr),use_osc(true)
{
  add(box);
}


source_panel_t::source_panel_t(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade)
  : Gtk::ScrolledWindow(cobject),use_osc(false)
{
  add(box);
}

void source_panel_t::update()
{
  for( unsigned int k=0;k<vbuttons.size();k++){
    vbuttons[k]->update();
  }
}

void source_panel_t::invalidate_win()
{
  for( unsigned int k=0;k<vbuttons.size();k++){
    vbuttons[k]->invalidate_win();
  }
}

void source_panel_t::set_scene( TASCAR::Scene::scene_t* s, TASCAR::session_t* session )
{
  for( unsigned int k=0;k<vbuttons.size();k++){
    box.remove(*(vbuttons[k]));
    delete vbuttons[k];
  }
  vbuttons.clear();
  if( s ){
    //std::vector<TASCAR::Scene::object_t*> obj(s->get_objects());
    for(std::vector<TASCAR::Scene::object_t*>::iterator it=s->all_objects.begin();it!=s->all_objects.end();++it){
      if( use_osc )
        vbuttons.push_back(new source_ctl_t(client_addr_,s,*it));
      else
        vbuttons.push_back(new source_ctl_t(s,*it));
    }
  }
  if( session ){
    for(std::vector<TASCAR::module_t*>::iterator it=session->modules.begin();
        it!= session->modules.end();++it){
      TASCAR::Scene::route_t* rp(dynamic_cast<TASCAR::Scene::route_t*>((*it)->libdata));
      if( rp ){
        if( use_osc )
          vbuttons.push_back(new source_ctl_t(client_addr_,s,rp));
        else
          vbuttons.push_back(new source_ctl_t(s,rp));
      }
    }
  }    

  for( unsigned int k=0;k<vbuttons.size();k++){
    box.pack_start(*(vbuttons[k]), Gtk::PACK_SHRINK);
  }
  show_all();
}

#define TEST_MODE(x) if( mode==#x ) lmode = dameter_t::x
void source_panel_t::set_levelmeter_mode( const std::string& mode )
{
  lmode = dameter_t::rmspeak;
  TEST_MODE(peak);
  TEST_MODE(rms);
  TEST_MODE(percentile);
  for(std::vector<source_ctl_t*>::iterator it=vbuttons.begin();it!=vbuttons.end();++it)
    (*it)->set_levelmeter_mode( lmode );
}

void source_panel_t::set_levelmeter_range( float vmin, float range )
{
  for(std::vector<source_ctl_t*>::iterator it=vbuttons.begin();it!=vbuttons.end();++it)
    (*it)->set_levelmeter_range( vmin, range );
}


scene_draw_t::scene_draw_t()
  : scene_(NULL),
    time(0),
    selection(NULL),
    markersize(0.02),
    blink(false),
    b_print_labels(true),
    b_acoustic_model(false)
{
  pthread_mutex_init( &mtx, NULL );
}

scene_draw_t::~scene_draw_t()
{
  pthread_mutex_trylock( &mtx );
  pthread_mutex_unlock( &mtx );
  pthread_mutex_destroy( &mtx );
}

void scene_draw_t::set_scene( TASCAR::render_core_t* scene )
{
  pthread_mutex_lock( &mtx );
  scene_ = scene;
  if( scene_ )
    view.set_ref(scene_->guicenter);
  else
    view.set_ref(TASCAR::pos_t());
  pthread_mutex_unlock( &mtx );
}


void scene_draw_t::select_object(TASCAR::Scene::object_t* o)
{
  selection = o;
}

void scene_draw_t::set_viewport(const viewt_t& viewt)
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( scene_ ){
      view.set_ref(scene_->guicenter);
    }
    switch( viewt ){
    case xy :
      view.set_perspective(false);
      view.set_euler(zyx_euler_t());
      break;
    case xz :
      view.set_perspective(false);
      view.set_euler(zyx_euler_t(0,0,-0.5*M_PI));
      break;
    case yz :
      view.set_perspective(false);
      view.set_euler(zyx_euler_t(0.5*M_PI,0,-0.5*M_PI));
      break;
    case xyz :
      view.set_perspective(false);
      view.set_euler(zyx_euler_t(0.1*M_PI,0,-0.45*M_PI));
      break;
    case p :
      view.set_perspective(true);
      if( scene_ ){
        if( scene_->receivermod_objects.size() ){
          view.set_ref(scene_->receivermod_objects[0]->get_location());
          view.set_euler(scene_->receivermod_objects[0]->get_orientation());
        }
      }
      break;
    }
    pthread_mutex_unlock( &mtx );
  }
}

void scene_draw_t::draw_acousticmodel(Cairo::RefPtr<Cairo::Context> cr)
{
  // draw acoustic model:
  cr->save();
  cr->set_source_rgb(0,0,0);
  cr->set_line_width( 0.2*markersize );
  for(std::vector<TASCAR::Acousticmodel::receiver_graph_t*>::iterator irc=scene_->world->receivergraphs.begin();
      irc!=scene_->world->receivergraphs.end();++irc)
    for(std::vector<TASCAR::Acousticmodel::acoustic_model_t*>::iterator iam=(*irc)->acoustic_model.begin();
        iam != (*irc)->acoustic_model.end();++iam){
    if( (*iam)->receiver_->volumetric.is_null() && (*iam)->src_->active && 
        (*iam)->receiver_->active && 
        ((*iam)->src_->ismmin <= (*iam)->ismorder) && 
        ((*iam)->src_->ismmax >= (*iam)->ismorder) &&
        ((*iam)->receiver_->ismmin <= (*iam)->ismorder) && 
        ((*iam)->receiver_->ismmax >= (*iam)->ismorder) ){
      pos_t psrc(view((*iam)->position));
      pos_t prec(view((*iam)->receiver_->position));
      cr->save();
      double gain_color(std::min(1.0,std::max(0.0,(*iam)->get_gain())));
      if( gain_color < EPS )
        // sources with zero gain but active are shown in red:
        cr->set_source_rgba(1, 0, 0, 0.5 );
      else
        // regular sources are gray:
        cr->set_source_rgba( 0,0,0,0.1+0.9*gain_color);
      // mark sources as circle with cross:
      cr->arc(psrc.x, -psrc.y, markersize, 0, PI2 );
      cr->move_to(psrc.x-0.7*markersize,-psrc.y+0.7*markersize);
      cr->line_to(psrc.x+0.7*markersize,-psrc.y-0.7*markersize);
      cr->move_to(psrc.x-0.7*markersize,-psrc.y-0.7*markersize);
      cr->line_to(psrc.x+0.7*markersize,-psrc.y+0.7*markersize);
      cr->stroke();
      // draw source traces:
      //draw_edge(cr,prec,psrc);
      //draw_source_trace(cr,(*iam)->receiver_->position,(*iam)->src_,*iam);
      // gray line from source to receiver:
      //cr->set_source_rgba(0, 0, 0, std::min(1.0,(*iam)->get_gain()));
      // regular sources are gray:
      if( gain_color > EPS ){
        cr->set_source_rgba( 0,0.6,0,0.1+0.9*gain_color);
        draw_edge(cr,psrc,prec);
        cr->stroke();
      }
      // image source or primary source:
      if( (*iam)->ismorder > 0 ){
        // image source:
        //pos_t pcut(view(((TASCAR::Acousticmodel::mirrorsource_t*)((*iam)->src_))->p_cut));
        //cr->arc(pcut.x, -pcut.y, 0.6*markersize, 0, PI2 );
        //cr->fill();
        //draw_edge(cr,psrc,pcut);
        //cr->stroke();
        cr->save();
        char ctmp[1000];
        sprintf(ctmp,"%d",(*iam)->ismorder);
        //((TASCAR::Acousticmodel::mirrorsource_t*)((*iam)->src_))->reflector_->);
        if( gain_color < EPS )
          // sources with zero gain but active are shown in red:
          cr->set_source_rgba(1, 0, 0, 0.5 );
        else
          // regular sources are gray:
          cr->set_source_rgba( 0,0,0,0.1+0.9*gain_color );
        //cr->set_source_rgba(0, 0, 0, 0.4);
        cr->move_to( psrc.x+1.2*markersize, -psrc.y );
        cr->show_text( ctmp );
        cr->stroke();
        cr->restore();
      }else{
        // primary source:
        cr->arc(psrc.x, -psrc.y, markersize, 0, PI2 );
        cr->fill();
      }
      cr->restore();
    }
  }
  cr->restore();
}

void scene_draw_t::draw(Cairo::RefPtr<Cairo::Context> cr)
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( scene_ ){
      if( scene_->guitrackobject )
        view.set_ref(scene_->guitrackobject->c6dof.position);
      //std::vector<TASCAR::Scene::object_t*> objects(scene_->get_objects());
      for(uint32_t k=0;k<scene_->all_objects.size();k++)
        draw_object(scene_->all_objects[k],cr );
      if( b_acoustic_model && scene_->world ){
        draw_acousticmodel(cr);
      }
    }
    pthread_mutex_unlock( &mtx );
  }
}

void scene_draw_t::draw_object(TASCAR::Scene::object_t* obj,Cairo::RefPtr<Cairo::Context> cr)
{
  if( !b_acoustic_model )
    draw_track(obj,cr,markersize);
  if( !b_acoustic_model )
    draw_src(dynamic_cast<TASCAR::Scene::src_object_t*>(obj),cr,markersize);
  draw_receiver_object(dynamic_cast<TASCAR::Scene::receivermod_object_t*>(obj),cr,markersize);
  //draw_door_src(dynamic_cast<TASCAR::Scene::src_door_t*>(obj),cr,markersize);
  draw_room_src(dynamic_cast<TASCAR::Scene::diffuse_info_t*>(obj),cr,markersize);
  draw_face(dynamic_cast<TASCAR::Scene::face_object_t*>(obj),cr,markersize);
  draw_facegroup(dynamic_cast<TASCAR::Scene::face_group_t*>(obj),cr,markersize);
  draw_obstaclegroup(dynamic_cast<TASCAR::Scene::obstacle_group_t*>(obj),cr,markersize);
  draw_mask(dynamic_cast<TASCAR::Scene::mask_object_t*>(obj),cr,markersize);
}

void scene_draw_t::ngon_draw_normal(TASCAR::ngon_t* f, Cairo::RefPtr<Cairo::Context> cr, double normalsize)
{
  if( f ){
    const std::vector<pos_t>& roomnodes(f->get_verts());
    pos_t center;
    for(unsigned int k=0;k<roomnodes.size();k++){
      center += roomnodes[k];
    }
    center *= 1.0/roomnodes.size();
    cr->save();
    if( normalsize > 0 ){
      pos_t pn(f->get_normal());
      pn *= normalsize;
      pn += center;
      pn = view(pn);
      center = view(center);
      draw_edge(cr,center,pn);
    }
    cr->stroke();
    cr->restore();
  }
}

void scene_draw_t::ngon_draw(TASCAR::ngon_t* f, Cairo::RefPtr<Cairo::Context> cr,bool fill, bool area)
{
  if( f ){
    std::vector<pos_t> roomnodes(f->get_verts());
    pos_t center;
    for(unsigned int k=0;k<roomnodes.size();k++){
      center += roomnodes[k];
      roomnodes[k] = view(roomnodes[k]);
    }
    center *= 1.0/roomnodes.size();
    center = view(center);
    cr->save();
    bool no_inf(true);
    for(unsigned int k=0;k<roomnodes.size()-1;k++){
      if( !draw_edge(cr,roomnodes[k],roomnodes[k+1]) )
        no_inf = false;
    }
    draw_edge(cr,roomnodes.back(),roomnodes[0]);
    cr->stroke();
    if( fill & (no_inf) ){
      cr->move_to(roomnodes[0].x, -roomnodes[0].y );
      for(unsigned int k=1;k<roomnodes.size();k++)
        cr->line_to( roomnodes[k].x, -roomnodes[k].y );
      cr->fill();
    }
    if( area ){
      char ctmp[1000];
      sprintf(ctmp,"%g m^2 (%g m)",f->get_area(),f->get_aperture());
      cr->move_to( center.x, -center.y );
      cr->show_text( ctmp );
      cr->stroke();
    }
    cr->restore();
  }
}

void scene_draw_t::draw_cube(TASCAR::pos_t pos, TASCAR::zyx_euler_t orient, TASCAR::pos_t size,Cairo::RefPtr<Cairo::Context> cr)
{
  std::vector<pos_t> roomnodes(8,pos_t());
  roomnodes[0].x -= 0.5*size.x;
  roomnodes[1].x += 0.5*size.x;
  roomnodes[2].x += 0.5*size.x;
  roomnodes[3].x -= 0.5*size.x;
  roomnodes[4].x -= 0.5*size.x;
  roomnodes[5].x += 0.5*size.x;
  roomnodes[6].x += 0.5*size.x;
  roomnodes[7].x -= 0.5*size.x;
  roomnodes[0].y -= 0.5*size.y;
  roomnodes[1].y -= 0.5*size.y;
  roomnodes[2].y += 0.5*size.y;
  roomnodes[3].y += 0.5*size.y;
  roomnodes[4].y -= 0.5*size.y;
  roomnodes[5].y -= 0.5*size.y;
  roomnodes[6].y += 0.5*size.y;
  roomnodes[7].y += 0.5*size.y;
  roomnodes[0].z -= 0.5*size.z;
  roomnodes[1].z -= 0.5*size.z;
  roomnodes[2].z -= 0.5*size.z;
  roomnodes[3].z -= 0.5*size.z;
  roomnodes[4].z += 0.5*size.z;
  roomnodes[5].z += 0.5*size.z;
  roomnodes[6].z += 0.5*size.z;
  roomnodes[7].z += 0.5*size.z;
  for(unsigned int k=0;k<8;k++)
    roomnodes[k] *= orient;
  for(unsigned int k=0;k<8;k++)
    roomnodes[k] += pos;
  for(unsigned int k=0;k<8;k++)
    roomnodes[k] = view(roomnodes[k]);
  cr->save();
  draw_edge(cr,roomnodes[0],roomnodes[1]);
  draw_edge(cr,roomnodes[1],roomnodes[2]);
  draw_edge(cr,roomnodes[2],roomnodes[3]);
  draw_edge(cr,roomnodes[3],roomnodes[0]);
  draw_edge(cr,roomnodes[4],roomnodes[5]);
  draw_edge(cr,roomnodes[5],roomnodes[6]);
  draw_edge(cr,roomnodes[6],roomnodes[7]);
  draw_edge(cr,roomnodes[7],roomnodes[4]);
  for(unsigned int k=0;k<4;k++){
    draw_edge(cr,roomnodes[k],roomnodes[k+4]);
  }
  cr->stroke();
  cr->restore();
  
}

void scene_draw_t::draw_track(TASCAR::Scene::object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    bool solo(obj->get_solo());
    pos_t p0;
    cr->save();
    if( solo && blink ){
      cr->set_source_rgba(1,0,0,0.5);
      cr->set_line_width( 1.2*msize );
      for( TASCAR::track_t::const_iterator it=obj->location.begin();it!=obj->location.end();++it){
        pos_t p(view(it->second));
        if( it != obj->location.begin() )
          draw_edge( cr, p0,p );
        p0 = p;
      }
      cr->stroke();
    }
    cr->set_source_rgb(obj->color.r, obj->color.g, obj->color.b );
    if( solo && blink )
      cr->set_line_width( 0.3*msize );
    else
      cr->set_line_width( 0.1*msize );
    for( TASCAR::track_t::const_iterator it=obj->location.begin();it!=obj->location.end();++it){
      pos_t p(view(it->second));
      if( it != obj->location.begin() )
        draw_edge( cr, p0,p );
      p0 = p;
    }
    cr->stroke();
    cr->restore();
  }
}

void scene_draw_t::draw_src(TASCAR::Scene::src_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    bool active(obj->isactive(time));
    bool solo(obj->get_solo());
    std::vector<TASCAR::pos_t> sndpos;
    TASCAR::pos_t p(obj->get_location());
    TASCAR::pos_t dir(0.03*view.scale,0,0);
    dir *= obj->get_orientation();
    dir += p;
    TASCAR::pos_t center(p);
    for(unsigned int k=0;k<obj->sound.size();k++){
      TASCAR::pos_t ptmp(obj->sound[k]->position);
      sndpos.push_back(view(ptmp));
      center += ptmp;
    }
    center *= 1.0/(1.0+sndpos.size());
    center = view(center);
    p = view(p);
    dir = view(dir);
    cr->save();
    if( p.z != std::numeric_limits<double>::infinity()){
      if( obj == selection ){
        cr->set_source_rgba(1, 0.7, 0, 0.5);
        cr->arc(p.x, -p.y, 3*msize, 0, PI2 );
        cr->fill();
      }
      if( active && solo && blink ){
        cr->set_source_rgba(1, 0, 0, 0.5);
        cr->arc(p.x, -p.y, 1.5*msize, 0, PI2 );
        cr->fill();
      }
      cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6+0.2*active);
      cr->arc(p.x, -p.y, msize, 0, PI2 );
      cr->fill();
      cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.3+0.5*active);
      cr->move_to( p.x, -p.y );
      draw_edge( cr, p, dir );
      cr->stroke();
    }
    for(unsigned int k=0;k<sndpos.size();k++){
      if( sndpos[k].z != std::numeric_limits<double>::infinity()){
        if( solo && blink ){
          cr->set_source_rgba(1, 0, 0, 0.5);
          cr->arc(sndpos[k].x, -sndpos[k].y, 0.75*msize, 0, PI2 );
          cr->fill();
        }
        cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
        cr->arc(sndpos[k].x, -sndpos[k].y, 0.5*msize, 0, PI2 );
        cr->fill();
      }
    }
    if( !active )
      cr->set_source_rgba(0, 0, 0, 0.75 );
    else
      cr->set_source_rgb(0, 0, 0 );
    if( b_print_labels ){
      if( center.z != std::numeric_limits<double>::infinity()){
        cr->move_to( center.x + 1.1*msize, -center.y );
        cr->show_text( obj->get_name().c_str() );
        cr->stroke();
      }
    }
    if( active ){
      cr->set_line_width( 0.1*msize );
      cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
      if( obj->sound.size()){
        pos_t pso(view(obj->sound[0]->position));
        if( pso.z != std::numeric_limits<double>::infinity()){
          for(unsigned int k=1;k<obj->sound.size();k++){
            pos_t ps(view(obj->sound[k]->position));
            bool view_x((fabs(ps.x)<1)||
                        (fabs(pso.x)<1));
            bool view_y((fabs(ps.y)<1)||
                        (fabs(pso.y)<1));
            if( view_x && view_y ){
              cr->move_to( pso.x, -pso.y );
              cr->line_to( ps.x, -ps.y );
              cr->stroke();
            }
            pso = ps;
          }
        }
      }
    }
    cr->restore();
  }
}

void scene_draw_t::draw_receiver_object(TASCAR::Scene::receivermod_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    if( view.get_perspective() )
      return;
    bool solo(obj->get_solo());
    msize *= 1.5;
    cr->save();
    cr->set_line_width( 0.2*msize );
    cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
    pos_t p(obj->get_location());
    zyx_euler_t o(obj->get_orientation());
    if( (obj->volumetric.x!=0)&&(obj->volumetric.y!=0)&&(obj->volumetric.z!=0) ){
      draw_cube(p,o,obj->volumetric,cr);
      if( obj->falloff > 0 ){
        std::vector<double> dash(2);
        dash[0] = msize;
        dash[1] = msize;
        cr->set_dash(dash,0);
        draw_cube(p,o,obj->volumetric+pos_t(2*obj->falloff,2*obj->falloff,2*obj->falloff),cr);
        dash[0] = 1.0;
        dash[1] = 0.0;
        cr->set_dash(dash,0);
      }
    }
    if( obj->delaycomp > 0 ){
      cr->save();
      double dr(obj->delaycomp*scene_->c);
      cr->set_source_rgba(1.0,0.0,0.0,0.6);
      std::vector<double> dash(2);
      dash[0] = msize;
      dash[1] = msize;
      cr->set_dash(dash,0);
      pos_t x(p);
      for(uint32_t k=0;k<25;k++){
        x = p;
        x.x += dr*cos(PI2*k/24.0);
        x.y += dr*sin(PI2*k/24.0);
        x = view(x);
        if( k==0 )
          cr->move_to(x.x,-x.y);
        else
          cr->line_to(x.x,-x.y);
      }
      for(uint32_t k=0;k<25;k++){
        x = p;
        x.y += dr*cos(PI2*k/24.0);
        x.z += dr*sin(PI2*k/24.0);
        x = view(x);
        if( k==0 )
          cr->move_to(x.x,-x.y);
        else
          cr->line_to(x.x,-x.y);
      }
      for(uint32_t k=0;k<25;k++){
        x = p;
        x.z += dr*cos(PI2*k/24.0);
        x.x += dr*sin(PI2*k/24.0);
        x = view(x);
        if( k==0 )
          cr->move_to(x.x,-x.y);
        else
          cr->line_to(x.x,-x.y);
      }
      cr->stroke();
      cr->restore();
    }
    double scale(0.5*view.get_scale());
    pos_t p1(1.8*msize*scale,-0.6*msize*scale,0);
    pos_t p2(2.9*msize*scale,0,0);
    pos_t p3(1.8*msize*scale,0.6*msize*scale,0);
    pos_t p4(-0.5*msize*scale,2.3*msize*scale,0);
    pos_t p5(0,1.7*msize*scale,0);
    pos_t p6(0.5*msize*scale,2.3*msize*scale,0);
    pos_t p7(-0.5*msize*scale,-2.3*msize*scale,0);
    pos_t p8(0,-1.7*msize*scale,0);
    pos_t p9(0.5*msize*scale,-2.3*msize*scale,0);
    p1 *= o;
    p2 *= o;
    p3 *= o;
    p4 *= o;
    p5 *= o;
    p6 *= o;
    p7 *= o;
    p8 *= o;
    p9 *= o;
    p1+=p;
    p2+=p;
    p3+=p;
    p4+=p;
    p5+=p;
    p6+=p;
    p7+=p;
    p8+=p;
    p9+=p;
    p = view(p);
    p1 = view(p1);
    p2 = view(p2);
    p3 = view(p3);
    p4 = view(p4);
    p5 = view(p5);
    p6 = view(p6);
    p7 = view(p7);
    p8 = view(p8);
    p9 = view(p9);
    if( obj == selection ){
      cr->set_source_rgba(1, 0.7, 0, 0.5);
      cr->arc(p.x, -p.y, 3*msize, 0, PI2 );
      cr->fill();
    }
    if( solo && blink ){
      cr->set_source_rgba(1, 0, 0, 0.5);
      cr->arc(p.x, -p.y, 1.5*msize, 0, PI2 );
      cr->fill();
    }
    cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
    cr->move_to( p.x, -p.y );
    cr->arc(p.x, -p.y, 2*msize, 0, PI2 );
    cr->move_to( p1.x, -p1.y );
    cr->line_to( p2.x, -p2.y );
    cr->line_to( p3.x, -p3.y );
    cr->move_to( p4.x, -p4.y );
    cr->line_to( p5.x, -p5.y );
    cr->line_to( p6.x, -p6.y );
    cr->move_to( p7.x, -p7.y );
    cr->line_to( p8.x, -p8.y );
    cr->line_to( p9.x, -p9.y );
    cr->stroke();
    if( obj->boundingbox.active ){
      cr->set_line_width( 0.1*msize );
      pos_t p(obj->boundingbox.get_location());
      zyx_euler_t o(obj->boundingbox.get_orientation());
      draw_cube(p,o,obj->boundingbox.size,cr);
      if( obj->boundingbox.falloff > 0 ){
        std::vector<double> dash(2);
        dash[0] = msize;
        dash[1] = msize;
        cr->set_dash(dash,0);
        draw_cube(p,o,obj->boundingbox.size+pos_t(2*obj->boundingbox.falloff,2*obj->boundingbox.falloff,2*obj->boundingbox.falloff),cr);
        dash[0] = 1.0;
        dash[1] = 0.0;
        cr->set_dash(dash,0);
      }
    }
    if( b_print_labels ){
      cr->set_source_rgb(0, 0, 0 );
      cr->move_to( p.x + 3.1*msize, -p.y );
      cr->show_text( obj->get_name().c_str() );
      cr->stroke();
    }
    cr->restore();
  }
}

void scene_draw_t::draw_room_src(TASCAR::Scene::diffuse_info_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    bool solo(obj->get_solo());
    pos_t p(obj->get_location());
    zyx_euler_t o(obj->get_orientation());
    cr->save();
    if( obj == selection ){
      cr->set_line_width( 2*msize );
      cr->set_source_rgba(1, 0.7, 0, 0.5);
      draw_cube(p,o,obj->size,cr);
    }
    if( solo && blink )
      cr->set_line_width( 0.6*msize );
    else
      cr->set_line_width( 0.1*msize );
    cr->set_source_rgb(obj->color.r, obj->color.g, obj->color.b );
    draw_cube(p,o,obj->size,cr);
    std::vector<double> dash(2);
    dash[0] = msize;
    dash[1] = msize;
    cr->set_dash(dash,0);
    pos_t falloff(obj->falloff,obj->falloff,obj->falloff);
    falloff *= 2.0;
    falloff += obj->size;
    draw_cube(p,o,falloff,cr);
    p = view(p);
    if( b_print_labels ){
      cr->set_source_rgb(0, 0, 0 );
      if( p.z != std::numeric_limits<double>::infinity()){
        cr->move_to( p.x, -p.y );
        cr->show_text( obj->get_name().c_str() );
        cr->stroke();
      }
    }
    cr->restore();
  }
}

void scene_draw_t::draw_face(TASCAR::Scene::face_object_t* face,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( face ){
    bool active(face->isactive(time));
    bool solo(face->get_solo());
    if( !active )
      msize*=0.5;
    pos_t loc(view(face->get_location()));
    cr->save();
    if( ((TASCAR::Scene::object_t*)face) == selection ){
      cr->set_line_width( 2*msize );
      cr->set_source_rgba(1,0.7,0,0.5);
      ngon_draw(face,cr);
    }
    if( solo && blink ){
      // solo indicating:
      cr->set_line_width( 1.2*msize );
      cr->set_source_rgba(1,0,0,0.5);
      ngon_draw(face,cr);
    }
    // outline:
    cr->set_line_width( 0.2*msize );
    if( active )
      cr->set_source_rgb(face->color.r,face->color.g,face->color.b);
    else
      cr->set_source_rgba(face->color.r,face->color.g,face->color.b,0.6);
    ngon_draw(face,cr);
    // fill:
    if( active ){
      cr->save();
      cr->set_line_width( 0.1*msize );
      ngon_draw_normal(face,cr,1.0);
      cr->restore();
      // normal and name:
      cr->set_source_rgba(face->color.r,face->color.g,0.5+0.5*face->color.b,0.3);
      if( loc.z != std::numeric_limits<double>::infinity()){
        if( !b_acoustic_model ){
          cr->arc(loc.x, -loc.y, msize, 0, PI2 );
          cr->fill();
        }
        cr->set_line_width( 0.4*msize );
        cr->set_source_rgba(face->color.r,face->color.g,face->color.b,GUI_FACE_ALPHA);
        ngon_draw(face,cr,true);
        if( b_print_labels && (!b_acoustic_model) ){
          cr->set_source_rgb(0, 0, 0 );
          cr->move_to( loc.x + 0.1*msize, -loc.y );
          cr->show_text( face->get_name().c_str() );
          cr->stroke();
        }
      }
    }
    cr->restore();
    
  }
}

void scene_draw_t::draw_facegroup(TASCAR::Scene::face_group_t* face,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( face ){
    bool active(face->isactive(time));
    bool solo(face->get_solo());
    if( !active )
      msize*=0.5;
    pos_t loc(view(face->get_location()));
    cr->save();
    if( ((TASCAR::Scene::object_t*)face) == selection ){
      cr->set_line_width( 2*msize );
      cr->set_source_rgba(1,0.7,0,0.5);
      for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
        ngon_draw(*it,cr);
    }
    if( solo && blink ){
      // solo indicating:
      cr->set_line_width( 1.5*msize );
      cr->set_source_rgba(1,0,0,0.5);
      for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
        ngon_draw(*it,cr);
    }
    // fill:
    if( active ){
      // normal and name:
      cr->set_source_rgba(face->color.r,face->color.g,0.5+0.5*face->color.b,0.3);
      if( loc.z != std::numeric_limits<double>::infinity()){
        if( !b_acoustic_model ){
          cr->arc(loc.x, -loc.y, msize, 0, PI2 );
          cr->fill();
        }
        cr->set_line_width( 0.4*msize );
        cr->set_source_rgba(face->color.r,face->color.g,face->color.b,GUI_FACE_ALPHA);
        for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
          ngon_draw(*it,cr,true);
        cr->save();
        cr->set_line_width( 0.2*msize );
        cr->set_source_rgb(face->color.r,face->color.g,face->color.b);
        for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
          ngon_draw_normal(*it,cr,0.2);
        cr->restore();
        //ngon_draw_normal(face,cr);
        if( b_print_labels && (!b_acoustic_model) ){
          cr->set_source_rgb(0, 0, 0 );
          cr->move_to( loc.x + 0.1*msize, -loc.y );
          cr->show_text( face->get_name().c_str() );
          cr->stroke();
        }
      }
    }else{
      // outline:
      cr->set_line_width( 0.2*msize );
      cr->set_source_rgba(face->color.r,face->color.g,face->color.b,0.6);
      for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
        ngon_draw(*it,cr);
      cr->save();
      cr->set_line_width( 0.1*msize );
      for(std::vector<TASCAR::Acousticmodel::reflector_t*>::iterator it=face->reflectors.begin();it!=face->reflectors.end();++it)
        ngon_draw_normal(*it,cr,0.2);
      cr->restore();
    }
    cr->restore();
    
  }
}

void scene_draw_t::draw_obstaclegroup(TASCAR::Scene::obstacle_group_t* face,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( face ){
    bool active(face->isactive(time));
    bool solo(face->get_solo());
    if( !active )
      msize*=0.5;
    pos_t loc(view(face->get_location()));
    cr->save();
    if( ((TASCAR::Scene::object_t*)face) == selection ){
      cr->set_line_width( 2*msize );
      cr->set_source_rgba(1,0.7,0,0.5);
      for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
        ngon_draw(*it,cr);
    }
    if( solo && blink ){
      // solo indicating:
      cr->set_line_width( 1.5*msize );
      cr->set_source_rgba(1,0,0,0.5);
      for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
        ngon_draw(*it,cr);
    }
    // fill:
    if( active ){
      // normal and name:
      cr->set_source_rgba(face->color.r,face->color.g,0.5+0.5*face->color.b,0.3);
      if( loc.z != std::numeric_limits<double>::infinity()){
        if( !b_acoustic_model ){
          cr->arc(loc.x, -loc.y, msize, 0, PI2 );
          cr->fill();
        }
        cr->set_line_width( 0.4*msize );
        cr->set_source_rgba(face->color.r,face->color.g,face->color.b,GUI_FACE_ALPHA);
        for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
          ngon_draw(*it,cr,true);
        //cr->save();
        //cr->set_line_width( 0.2*msize );
        //cr->set_source_rgb(face->color.r,face->color.g,face->color.b);
        //for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
        //  ngon_draw_normal(*it,cr,0.2);
        //cr->restore();
        //ngon_draw_normal(face,cr);
        if( b_print_labels && (!b_acoustic_model) ){
          cr->set_source_rgb(0, 0, 0 );
          cr->move_to( loc.x + 0.1*msize, -loc.y );
          cr->show_text( face->get_name().c_str() );
          cr->stroke();
        }
      }
    }else{
      // outline:
      cr->set_line_width( 0.2*msize );
      cr->set_source_rgba(face->color.r,face->color.g,face->color.b,0.6);
      for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
        ngon_draw(*it,cr);
      cr->save();
      cr->set_line_width( 0.1*msize );
      for(std::vector<TASCAR::Acousticmodel::obstacle_t*>::iterator it=face->obstacles.begin();it!=face->obstacles.end();++it)
        ngon_draw_normal(*it,cr,0.2);
      cr->restore();
    }
    cr->restore();
    
  }
}

void scene_draw_t::draw_mask(TASCAR::Scene::mask_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    if( view.get_perspective() )
      return;
    msize *= 1.5;
    cr->save();
    cr->set_line_width( 0.2*msize );
    cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
    if( obj->mask_inner ){
      draw_cube(obj->center,obj->shoebox_t::orientation,obj->xmlsize+pos_t(2*obj->xmlfalloff,2*obj->xmlfalloff,2*obj->xmlfalloff),cr);
      std::vector<double> dash(2);
      dash[0] = msize;
      dash[1] = msize;
      cr->set_dash(dash,0);
      draw_cube(obj->center,obj->shoebox_t::orientation,obj->xmlsize,cr);
      dash[0] = 1.0;
      dash[1] = 0.0;
      cr->set_dash(dash,0);
    }else{
      draw_cube(obj->center,obj->shoebox_t::orientation,obj->xmlsize,cr);
      std::vector<double> dash(2);
      dash[0] = msize;
      dash[1] = msize;
      cr->set_dash(dash,0);
      draw_cube(obj->center,obj->shoebox_t::orientation,obj->xmlsize+pos_t(2*obj->xmlfalloff,2*obj->xmlfalloff,2*obj->xmlfalloff),cr);
      dash[0] = 1.0;
      dash[1] = 0.0;
      cr->set_dash(dash,0);
    }
    cr->restore();
  }
}

void scene_draw_t::set_markersize(double msize)
{
  markersize = msize;
}

void scene_draw_t::set_blink(bool blink_)
{
  blink = blink_;
}

void scene_draw_t::set_time(double t)
{
  time = t;
}

void scene_draw_t::set_print_labels(bool print_labels)
{
  b_print_labels = print_labels;
}

void scene_draw_t::set_show_acoustic_model(bool acmodel)
{
  b_acoustic_model = acmodel;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
