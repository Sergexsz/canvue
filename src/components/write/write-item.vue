<template>
  <tr>
    <td><input type="text" v-model="out.id"></td>
    <td><input type="text" v-model="out.count"></td>
    <td><input @change="setPeriod" type="text" v-model="out.period"></td>
    <td><input type="number" v-model="out.dlc"></td>
    <template :key="n" v-for="n in 8">
      <td>
        <input type="text" v-model="out.data[n-1]">
      </td>
    </template>
    <td><textarea v-model="out.comment"></textarea></td>
    <td>
      <button class="btn" @click="$emit('delete')">Удалить</button>
      <button class="btn" @click="$emit('write')">Отправить</button>
    </td>
  </tr>
</template>

<script>
export default {
  name: "write-item",
  props: ['value'],
  data() {
    return {
      interval: null
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
      this.interval = setInterval(this.write, this.value.period);
      console.log(this.interval);
    },
    write() {
      if (this.value.period > 0)
        this.$emit('write');
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