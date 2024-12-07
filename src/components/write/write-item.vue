<template>
  <tr :class="{
    'bg-success-subtle': interval,
    'bg-warning-subtle': outPack.period > 0 && !interval
  }">
    <td>
      <el-input class="fw-bold el-input--small" type="text"
                v-model="outPack.id"></el-input>
    </td>
    <td>
      <el-input
          class=" el-input--small"
          type="number"
          :max="8"
          v-model="outPack.dlc"></el-input>
    </td>
    <template :key="n" v-for="n in 8">
      <td style="width: 40px">
        <el-input
            class="el-input--small"
            v-if="n <= outPack.dlc"
            type="text" v-model="outPack.data[n-1]"></el-input>
      </td>
    </template>
    <td>
      <div class="ellipsis w-100 small" @click="editComment">
        <template v-if="outPack.comment"> {{ outPack.comment }}</template>
        <span class="text-muted" v-else>указать...</span>
      </div>
    </td>
    <td>
      <el-select
          clearable
          class="w-100 el-select--small"
          v-model="outPack.period"
          @change="setPeriod;$emit('input');">
        <el-option v-for="period in periods"
                   :key="period"
                   :value="period"
                   :label="period"></el-option>
      </el-select>
    </td>
    <td>
      <el-input class=" el-input--small"
                type="text"
                clearable
                @clear="$emit('input')"
                v-model="outPack.count"></el-input>
    </td>
    <td>
      <div class="d-flex justify-content-between">
        <button class="btn btn-danger d-inline-flex align-items-center btn-sm px-1 py-0 border-0"
                v-if="!interval"
                :disabled="!outPack.period || outPack.period < 1"
                @click="setPeriod"><el-icon><TurnOff /></el-icon>
        </button>
        <button class="btn btn-success d-inline-flex align-items-center  btn-sm px-1 py-0 border-0"
                v-if="interval"
                @click="stopPeriod"><el-icon><Open /></el-icon>
        </button>
        <button class="btn btn-info btn-sm  px-1 py-0 border-0 ms-2" @click="$emit('written')">Отправить</button>
        <button class="btn btn-outline-danger btn-sm  px-1 py-0 ms-3 border-0" @click="deleteIt()">Удалить</button>

      </div>
    </td>
  </tr>

</template>

<script>
import {ElMessageBox} from "element-plus";

export default {
  name: "write-item",
  props: ['value'],
  emits: ['written', 'input', 'delete'],
  data() {
    return {
      interval: null,
      periods: [0, 100, 200, 300, 400, 500, 1000, 2000, 5000]
    }
  },
  model: {
    prop: "value",
    event: 'input'
  },
  methods: {
    // установить период отправки
    setPeriod() {
      this.stopPeriod(); // clear old
      if (this.outPack.period)
        this.interval = setInterval(this.write, this.outPack.period); // set new
    },
    stopPeriod() {
      if (this.interval) {
        clearInterval(this.interval); // clear
        this.interval = null;
      }
    },
    // отправить в шину
    write() {
      if (this.outPack.period > 99)
        this.$emit('written');
    },
    //редактировать описание
    editComment() {
      ElMessageBox.prompt('введите комментарий', {
        inputValue: this.outPack.comment
      }).then(r => {
        this.outPack.comment = r.value;
        this.$emit('input');
      }).catch(() => {
      })
    },
    deleteIt() {
      ElMessageBox.confirm('Удалить?',
          {}).then(() => {
        this.$emit('delete')
      }).catch(() => {
      })
    }
  },
  unmounted() {
    this.stopPeriod()
  },
  computed: {
    outPack() {
      return this.value;
    }
    // outPack: {
    //   get() {
    //     return this.value;
    //   },
    //   set(val) {
    //     this.$emit('save', val);
    //   }
    // }
  }
}
</script>

<style scoped>

</style>