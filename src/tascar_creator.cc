#include "scene.h"

using namespace TASCAR;

int main(int argc,char**argv)
{
    scene_t scene;
    scene.description = "dummy scene";
    scene.src.push_back(src_object_t());
    scene.src.push_back(src_object_t());
    scene.bg_amb.push_back(bg_amb_t());
    scene.bg_amb[0].start = 88;
    scene.bg_amb[0].filename = "afile_amb.wav";
    scene.bg_amb.push_back(bg_amb_t());
    scene.src[0].position[0] = pos_t(1,2,3);
    scene.src[0].position[3.7] = pos_t(4.1,5.2,6.3);
    scene.src[0].start = 77;
    scene.src[0].name = "an object";
    scene.src[0].sound.filename = "soundfile.wav";
    scene.src[0].sound.gain = -4.5;
    scene.src[0].sound.channel = 6;
    scene.src[0].sound.loop = 48;
    xml_write_scene("temp.tsc",scene," created with tascar_creator ");
    scene_t s_n = xml_read_scene("temp.tsc");
    xml_write_scene("temp1.tsc",s_n," created with tascar_creator ");
    
    return 0;
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */
