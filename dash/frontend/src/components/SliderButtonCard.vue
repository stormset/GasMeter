<template>
  <div class="column is-12-mobile is-4-tablet is-4-desktop is-3-fullhd">
      <div class="card">
        <span class="dot" :class="{'active': activity}"></span>
        <div class="card-content">
          <div class="columns is-mobile is-vcentered">
            <div @click="toggleShow" class="column has-text-centered">
              <h6 class="is-size-6 has-text-muted">{{card.name}}</h6>
            </div>
            <div class="column is-narrow">
              <transition name="btn-fade" mode="out-in">
                <div class="card-icon pb-3 pt-4 px-5 has-background-success has-text-light" style="cursor: pointer;" v-if="card.value == 1" @click="sendClickEvent" key="active">
                  <svg width="24" height="24" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><g fill="none" fill-rule="evenodd"><path d="M0 0h24v24H0z"></path><path d="M16.769 7.818a1 1 0 011.462 1.364l-7 7.5a1 1 0 01-1.382.077l-3.5-3a1 1 0 011.302-1.518l2.772 2.376 6.346-6.8z" fill="currentColor"></path></g></svg>
                </div>
                <div class="card-icon pb-3 pt-4 px-5 has-background-grey-dark has-text-white" style="cursor: pointer;" v-else @click="sendClickEvent" key="idle">
                  <svg width="24" height="24" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><g fill="none" fill-rule="evenodd"><path d="M0 0h24v24H0z"></path><path d="M12.036 10.621l2.828-2.828a1 1 0 011.414 1.414l-2.828 2.829 2.828 2.828a1 1 0 01-1.414 1.414l-2.828-2.828-2.829 2.828a1 1 0 11-1.414-1.414l2.828-2.828-2.828-2.829a1 1 0 011.414-1.414l2.829 2.828z" fill="currentColor"></path></g></svg>
                </div>
              </transition>
            </div>
          </div>
          <div class="dropdown">
            <span @click="toggleShow" class="noselect" :class="{'open': show}">&#10095;</span>
            <slide-up-down :active="show">
                  <div>
                    <label for="duration">Timer:</label>
                    <div>
                      <vue-slide-bar v-model="slider.value"
                                :data="slider.data"
                                :range="slider.range"
                                :lineHeight="16"
                                @input="callbackInput"
                                @callbackRange="callbackRange">
                                <template slot="tooltip">
                                    <span class="vue-slide-bar-tooltip">
                                      {{ getLabel() }}
                                    </span>
                                </template>
                      </vue-slide-bar>
                    </div>
                  </div>
            </slide-up-down>
          </div>
        </div>
      </div>
    </div>
</template>

<script>
import Vue from 'vue'
import EventBus from '@/event-bus.js';
import SlideUpDown from 'vue-slide-up-down'
import VueSlideBar from 'vue-slide-bar'

Vue.component('vue-slide-bar', VueSlideBar);
Vue.component('slide-up-down', SlideUpDown);


const range = (start, end, step = 1) => {
  let output = [];
  if (typeof end === 'undefined') {
    end = start;
    start = 0;
  }
  for (let i = start; i < end; i += step) {
    output.push(i);
  }
  return output;
};

const timeString = (sliderValue) => {
  const hours = Math.floor(sliderValue / 60);
  const minutes = sliderValue - hours * 60;
  const hourSuff = (hours === 1 ? "hr" : "hrs")
  const minSuff = (minutes === 1 ? "min" : "mins")

  if (hours === 0)
    return `${minutes} ${minSuff}`
  if(minutes === 0)
    return `${hours} ${hourSuff}`

  return `${hours} ${hourSuff} ${minutes} ${minSuff}`
}

export default {
    props:['card'],

    data(){
      const valueList = range(0, 245, 5);
      const labelList = range(0, 245, 10).map((v) => {
        if (v % 60 === 0) return { label: timeString(v) };

        return { label: timeString(v), isHide: true };
      });
      labelList[0].label = "Off";

      return {
        activity: true,
        show: false,
        slider: {
          value: 0,
          data: valueList,
          range: labelList,
        }
      }
    },

    methods:{
      sendClickEvent(){
        EventBus.$emit('buttonClicked', { id: this.card.id, value: !this.card.value });
        this.activity = true;
        setTimeout(() => { this.activity = false }, 100);
      },
      toggleShow() {
        this.show = !this.show;
      },
      getLabel() {
        return timeString(this.slider.value);
      },
      callbackRange (val) {
      },
      callbackInput(val) {
        this.msg = {
          "id": this.card.id,
          "value": val
        };

        EventBus.$emit('sliderChanged', this.msg);
      }
    },

    mounted(){
      setTimeout(() => { this.activity = false; this.$forceUpdate(); }, 500);
    }
}
</script>