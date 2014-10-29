#include "gui_elements.h"

void scene_draw_t::draw_edge(Cairo::RefPtr<Cairo::Context> cr, pos_t p1, pos_t p2)
{
  if( !(p1.has_infinity() || p2.has_infinity()) ){
    cr->move_to(p1.x,-p1.y);
    cr->line_to(p2.x,-p2.y);
  }
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
    tlabel.set_text("mir");
  if( dynamic_cast<TASCAR::Scene::src_object_t*>(route_))
    tlabel.set_text("src");
  if( dynamic_cast<TASCAR::Scene::src_diffuse_t*>(route_))
    tlabel.set_text("dif");
  if( dynamic_cast<TASCAR::Scene::sinkmod_object_t*>(route_))
    tlabel.set_text("sink");
  if( dynamic_cast<TASCAR::Scene::src_door_t*>(route_))
    tlabel.set_text("door");
  box.pack_start( tlabel, Gtk::PACK_SHRINK );
  box.pack_start( label, Gtk::PACK_EXPAND_PADDING );
  box.pack_start( mute, Gtk::PACK_SHRINK );
  box.pack_start( solo, Gtk::PACK_SHRINK );
  mute.set_active(route_->get_mute());
  solo.set_active(route_->get_solo());
#ifdef GTKMM30
  Gdk::RGBA col_yellow("f4e83a");
  col_yellow.set_rgba(244.0/256,232.0/256,58.0/256,0.5);
  //mute.override_background_color(col_yellow);
  //mute.override_color(col_yellow);
  //col_yellow.set_rgba_u(219*256,18*256,18*256);
  //solo.override_background_color(col_yellow,Gtk::STATE_FLAG_ACTIVE);
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
  add(ebox);
  mute.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_mute));
  solo.signal_clicked().connect(sigc::mem_fun(*this,&source_ctl_t::on_solo));
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

void source_ctl_t::on_solo()
{
  bool m(solo.get_active());
  if( use_osc ){
    std::string path("/"+scene_->name+"/"+name_+"/solo");
    lo_send(client_addr_,path.c_str(),"i",m);
  }else{
    //uint32_t anysolo(0);
    route_->set_solo(m,scene_->anysolo);
  }
}

source_panel_t::source_panel_t(lo_address client_addr)
  : client_addr_(client_addr),use_osc(true)
{
  set_size_request( 300, -1 );
  add(box);
}


source_panel_t::source_panel_t(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade)
  : Gtk::ScrolledWindow(cobject),use_osc(false)
{
  set_size_request( 300, -1 );
  add(box);
}

void source_panel_t::set_scene(TASCAR::Scene::scene_t* s)
{
  for( unsigned int k=0;k<vbuttons.size();k++){
    box.remove(*(vbuttons[k]));
    delete vbuttons[k];
  }
  vbuttons.clear();
  if( s ){
    std::vector<TASCAR::Scene::object_t*> obj(s->get_objects());
    for(std::vector<TASCAR::Scene::object_t*>::iterator it=obj.begin();it!=obj.end();++it){
      if( use_osc )
        vbuttons.push_back(new source_ctl_t(client_addr_,s,*it));
      else
        vbuttons.push_back(new source_ctl_t(s,*it));
    }
  }
  for( unsigned int k=0;k<vbuttons.size();k++){
    box.pack_start(*(vbuttons[k]), Gtk::PACK_SHRINK);
  }
  show_all();
}

scene_draw_t::scene_draw_t()
  : scene_(NULL),
    time(0),
    selection(NULL),
    markersize(0.02),
    blink(false)
{
  pthread_mutex_init( &mtx, NULL );
}

scene_draw_t::~scene_draw_t()
{
  pthread_mutex_trylock( &mtx );
  pthread_mutex_unlock( &mtx );
  pthread_mutex_destroy( &mtx );
}

void scene_draw_t::set_scene(TASCAR::Scene::scene_t* scene)
{
  pthread_mutex_lock( &mtx );
  scene_ = scene;
  pthread_mutex_unlock( &mtx );
}

void scene_draw_t::select_object(TASCAR::Scene::object_t* o)
{
  selection = o;
}

void scene_draw_t::draw(Cairo::RefPtr<Cairo::Context> cr,const viewt_t& viewt)
{
  if( pthread_mutex_lock( &mtx ) == 0 ){
    if( scene_ ){
      //scene_->geometry_update(time);
      switch( viewt ){
      case xy :
        view.set_perspective(false);
        view.set_ref(scene_->guicenter);
        view.set_euler(zyx_euler_t());
        break;
      case xz :
        view.set_perspective(false);
        view.set_ref(scene_->guicenter);
        view.set_euler(zyx_euler_t(0,-0.5*M_PI,0.5*M_PI));
        break;
      case yz :
        view.set_perspective(false);
        view.set_ref(scene_->guicenter);
        view.set_euler(zyx_euler_t(0,0,0.5*M_PI));
        break;
      case p :
        view.set_perspective(true);
        if( scene_->sinkmod_objects.size() ){
          view.set_ref(scene_->sinkmod_objects[0]->get_location());
          view.set_euler(scene_->sinkmod_objects[0]->get_orientation());
        }
        break;
      }
      std::vector<TASCAR::Scene::object_t*> objects(scene_->get_objects());
      for(uint32_t k=0;k<objects.size();k++)
        draw_object(objects[k],cr);
    }
    pthread_mutex_unlock( &mtx );
  }
}

void scene_draw_t::draw_object(TASCAR::Scene::object_t* obj,Cairo::RefPtr<Cairo::Context> cr)
{
  //bool selected(obj==selection);
  draw_track(obj,cr,markersize);
  draw_src(dynamic_cast<TASCAR::Scene::src_object_t*>(obj),cr,markersize);
  draw_sink_object(dynamic_cast<TASCAR::Scene::sinkmod_object_t*>(obj),cr,markersize);
  draw_door_src(dynamic_cast<TASCAR::Scene::src_door_t*>(obj),cr,markersize);
  draw_room_src(dynamic_cast<TASCAR::Scene::src_diffuse_t*>(obj),cr,markersize);
  draw_face(dynamic_cast<TASCAR::Scene::face_object_t*>(obj),cr,markersize);
  draw_mask(dynamic_cast<TASCAR::Scene::mask_object_t*>(obj),cr,markersize);
}

void scene_draw_t::draw_face_normal(TASCAR::face_t* f, Cairo::RefPtr<Cairo::Context> cr, double normalsize)
{
  if( f ){
    std::vector<pos_t> roomnodes(4,pos_t());
    roomnodes[0] = f->get_anchor();
    roomnodes[1] = f->get_anchor();
    roomnodes[1] += f->get_e1();
    roomnodes[2] = f->get_anchor();
    roomnodes[2] += f->get_e1();
    roomnodes[2] += f->get_e2();
    roomnodes[3] = f->get_anchor();
    roomnodes[3] += f->get_e2();
    for(unsigned int k=0;k<roomnodes.size();k++)
      roomnodes[k] = view(roomnodes[k]);
    cr->save();
    draw_edge(cr,roomnodes[0],roomnodes[1]);
    draw_edge(cr,roomnodes[1],roomnodes[2]);
    draw_edge(cr,roomnodes[2],roomnodes[3]);
    draw_edge(cr,roomnodes[3],roomnodes[0]);
    if( normalsize >= 0 ){
      pos_t pn(f->get_normal());
      pn *= normalsize;
      pn += f->get_anchor();
      pn = view(pn);
      draw_edge(cr,roomnodes[0],pn);
    }
    cr->stroke();
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
    double plot_time(time);
    if( !active ){
      msize *= 0.4;
      plot_time = std::min(std::max(plot_time,obj->starttime),obj->endtime);
    }
    pos_t p(obj->get_location());
    cr->save();
    p = view(p);
    if( p.z != std::numeric_limits<double>::infinity()){
      if( solo && blink ){
        cr->set_source_rgba(1, 0, 0, 0.5);
        cr->arc(p.x, -p.y, 3*msize, 0, PI2 );
        cr->fill();
      }
      cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
      cr->arc(p.x, -p.y, msize, 0, PI2 );
      cr->fill();
    }
    for(unsigned int k=0;k<obj->sound.size();k++){
      pos_t ps(view(obj->sound[k].get_pos_global(plot_time)));
      if( ps.z != std::numeric_limits<double>::infinity()){
        cr->arc(ps.x, -ps.y, 0.5*msize, 0, PI2 );
        cr->fill();
      }
    }
    if( !active )
      cr->set_source_rgb(0.5, 0.5, 0.5 );
    else
      cr->set_source_rgb(0, 0, 0 );
    if( p.z != std::numeric_limits<double>::infinity()){
      cr->move_to( p.x + 1.1*msize, -p.y );
      cr->show_text( obj->get_name().c_str() );
    }
    if( active ){
      cr->set_line_width( 0.1*msize );
      cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
      if( obj->sound.size()){
        pos_t pso(view(obj->sound[0].get_pos_global(plot_time)));
        if( pso.z != std::numeric_limits<double>::infinity()){
          for(unsigned int k=1;k<obj->sound.size();k++){
            pos_t ps(view(obj->sound[k].get_pos_global(plot_time)));
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

void scene_draw_t::draw_sink_object(TASCAR::Scene::sinkmod_object_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    if( view.get_perspective() )
      return;
    msize *= 1.5;
    cr->save();
    cr->set_line_width( 0.2*msize );
    cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6);
    pos_t p(obj->get_location());
    zyx_euler_t o(obj->get_orientation());
    //DEBUG(o.print());
    //o.z += headrot;
    if( (obj->size.x!=0)&&(obj->size.y!=0)&&(obj->size.z!=0) ){
      draw_cube(p,o,obj->size,cr);
      if( obj->falloff > 0 ){
        std::vector<double> dash(2);
        dash[0] = msize;
        dash[1] = msize;
        cr->set_dash(dash,0);
        draw_cube(p,o,obj->size+pos_t(2*obj->falloff,2*obj->falloff,2*obj->falloff),cr);
        dash[0] = 1.0;
        dash[1] = 0.0;
        cr->set_dash(dash,0);
      }
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
    //cr->fill();
    cr->stroke();
    if( obj->mask.active ){
      cr->set_line_width( 0.1*msize );
      pos_t p(obj->mask.get_location());
      zyx_euler_t o(obj->mask.get_orientation());
      draw_cube(p,o,obj->mask.size,cr);
      if( obj->mask.falloff > 0 ){
        std::vector<double> dash(2);
        dash[0] = msize;
        dash[1] = msize;
        cr->set_dash(dash,0);
        draw_cube(p,o,obj->mask.size+pos_t(2*obj->mask.falloff,2*obj->mask.falloff,2*obj->mask.falloff),cr);
        dash[0] = 1.0;
        dash[1] = 0.0;
        cr->set_dash(dash,0);
      }
    }
    cr->set_source_rgb(0, 0, 0 );
    cr->move_to( p.x + 3.1*msize, -p.y );
    cr->show_text( obj->get_name().c_str() );
    cr->restore();
  }
}

void scene_draw_t::draw_door_src(TASCAR::Scene::src_door_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    bool solo(obj->get_solo());
    pos_t p(obj->get_location());
    zyx_euler_t o(obj->get_orientation());
    o += obj->dorientation;
    cr->save();
    if( solo && blink )
      cr->set_line_width( 1.2*msize );
    else
      cr->set_line_width( 0.4*msize );
    cr->set_source_rgb(obj->color.r, obj->color.g, obj->color.b );
    face_t f;
    f.set(p,o,obj->width,obj->height);
    draw_face_normal(&f,cr);
    std::vector<double> dash(2);
    dash[0] = msize;
    dash[1] = msize;
    cr->set_dash(dash,0);
    cr->set_source_rgba(obj->color.r, obj->color.g, obj->color.b, 0.6 );
    f += obj->falloff;
    draw_face_normal(&f,cr);
    p = view(p);
    cr->set_source_rgb(0, 0, 0 );
    if( p.z != std::numeric_limits<double>::infinity()){
      cr->move_to( p.x, -p.y );
      cr->show_text( obj->get_name().c_str() );
    }
    cr->restore();
  }
}

void scene_draw_t::draw_room_src(TASCAR::Scene::src_diffuse_t* obj,Cairo::RefPtr<Cairo::Context> cr, double msize)
{
  if( obj ){
    bool solo(obj->get_solo());
    pos_t p(obj->get_location());
    zyx_euler_t o(obj->get_orientation());
    cr->save();
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
    cr->set_source_rgb(0, 0, 0 );
    if( p.z != std::numeric_limits<double>::infinity()){
      cr->move_to( p.x, -p.y );
      cr->show_text( obj->get_name().c_str() );
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
    if( solo && blink ){
      // solo indicating:
      cr->set_line_width( 1.2*msize );
      cr->set_source_rgba(1,0,0,0.5);
      draw_face_normal(face,cr);
    }
    // outline:
    cr->set_line_width( 0.2*msize );
    cr->set_source_rgba(face->color.r,face->color.g,face->color.b,0.6);
    draw_face_normal(face,cr,true);
    // fill:
    if( active ){
      // normal and name:
      cr->set_source_rgba(face->color.r,face->color.g,0.5+0.5*face->color.b,0.3);
      if( loc.z != std::numeric_limits<double>::infinity()){
        cr->arc(loc.x, -loc.y, msize, 0, PI2 );
        cr->fill();
        cr->set_line_width( 0.4*msize );
        cr->set_source_rgb(face->color.r,face->color.g,face->color.b);
        draw_face_normal(face,cr);
        cr->set_source_rgb(0, 0, 0 );
        cr->move_to( loc.x + 0.1*msize, -loc.y );
        cr->show_text( face->get_name().c_str() );
      }
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

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
