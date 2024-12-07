<template>
  <tr :class="{
    'bg-success-subtle': out.period > 0
  }">
    <td>
      <el-input class="fw-bold el-input--small" type="text" v-model="out.id"></el-input>
    </td>
    <td>
      <el-input
          class=" el-input--small"
          type="number"
          :max="8"
          v-model="out.dlc"></el-input>
    </td>
    <template :key="n" v-for="n in 8">
      <td style="width: 40px">
        <el-input
            class="el-input--small"
            v-if="n <= out.dlc"
            type="text" v-model="out.data[n-1]"></el-input>
      </td>
    </template>
    <td>
      <div class="ellipsis w-100 small" @click="editComment">
        <template v-if="out.comment"> {{ out.comment }}</template>
        <span class="text-muted" v-else>указать...</span>
      </div>
    </td>
    <td>
      <el-select
          clearable
          class="w-100 el-select--small"
          v-model="out.period" @change="setPeriod">
        <el-option v-for="period in periods"
                   :key="period"
                   :value="period"
                   :label="period"></el-option>
      </el-select>
    </td>
    <td>
      <el-input class=" el-input--small"
                rows="1"
                readonly type="text" v-model="out.count"></el-input>
    </td>
    <td>
      <div class="d-flex">
        <button class="btn btn-outline-danger btn-sm p-0 me-1 border-0" @click="$emit('delete')">Удалить</button>
        <button class="btn btn-outline-info btn-sm p-0 border-0" @click="$emit('write')">Отправить</button>
      </div>
    </td>
  </tr>

</template>

<script>
import {ElMessageBox} from "element-plus";

export default {
  name: "write-item",
  props: ['value'],
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
    setPeriod() {
      if (this.interval)
        clearInterval(this.interval);
      this.interval = setInterval(this.write, this.out.period);
      // console.log(this.interval);
    },
    write() {
      if (this.out.period > 99)
        this.$emit('write');
    },
    editComment() {
      ElMessageBox.prompt('введите комментарий',
          {
            inputValue: this.out.comment
          }).then(r => {
        console.log(r);
        this.out.content = r
      })
          .catch(() => {
          })
    }
  },
  computed: {
    out: {
      get() {
        return this.value;
      },
      set(val) {
        this.$emit('input', val);
      }
    }
  }
}
</script>

<style scoped>

</style>