<template>
  <tr>
    <td>{{ item.id }}</td>
    <td class="text-info text-center">{{ item.dlc }}</td>
    <!--          data-->
    <dlc-item
        v-for="n in data_length" :key="n"
        :data="item.data[n-1]"></dlc-item>
    <td>{{ item.count }}</td>
    <td>± {{ item.frequency }}мс</td>
    <!--          comment-->
    <td>
      <el-input
          class="el-input--small"
          v-model="device.comment"></el-input>
    </td>
  </tr>
</template>

<script>
import DlcItem from "@/components/dlc-item";
import _ from 'lodash'

export default {
  name: "table-inc-device",
  props: ['item', 'value', 'list'],
  model: {
    prop: 'value',
    event: 'input'
  },
  data() {
    return {
      data_length: 8
    }
  },
  computed: {
    device() {
      if (this.list) {
        let index = _.findIndex(this.list, {id: this.item.id});
        if (index > -1)
          return this.list[index]
      }

      return {};
    }
  },
  components: {DlcItem},
}
</script>

<style scoped>

</style>