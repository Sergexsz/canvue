<template>
  <td class="td fw-bold text-center"
      :class="is_changed ? 'active text-white':'text-success'"
      style="width: 40px"
      :style="'background: rgb(21 185 2 / '+ active_fill +'%);'">
    <template v-if="data">
      {{ data.value }}
    </template>
  </td>
</template>

<script>
import moment from 'moment'

export default {
  name: "dlc-item",
  props: ['data'],
  watch: {
    "data.value"(n, old) {
      if (n !== old)
        this.activateBit()
    },
  },
  data() {
    return {
      is_changed: false,
      active_interval: null,
      active_fill: 100
    }
  },
  computed: {
    changed() {
      // return  this.data.time;
      return moment(this.data.time, 'x').fromNow();
    }
  },
  created() {
    this.active_interval = setInterval(() => this.checkFill(), 100);
  },
  methods: {
    activateBit() {
      this.is_changed = true;
      this.active_fill = 100;
    },
    checkFill() {
      if (this.active_fill > 0) {
        this.active_fill = this.active_fill - 10;
      } else {
        this.is_changed = false;
      }
    }
  }
}
</script>

<style scoped>

td.td {
}

td.td {
  transition: background-color 0.2s ease-in-out;
}
</style>