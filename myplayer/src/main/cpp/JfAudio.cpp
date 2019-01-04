//
// Created by zjf on 2019/1/4.
//

#include "JfAudio.h"

JfAudio::JfAudio(JfPlayStatus *playStatus ) {
    this->playStatus = playStatus;
    queue = new JfQueue(playStatus);
}

JfAudio::~JfAudio() {

}
