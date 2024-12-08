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
    <td style="width: 20px;">
      <el-tooltip placement="left" content="Копировать текущее состояние в команды">
        <button class="btn btn-sm" @click="copyToWriteTable">
          <el-icon>
            <CopyDocument/>
          </el-icon>
        </button>
      </el-tooltip>
    </td>
  </tr>
</template>

<script>
import DlcItem from "@/components/dlc-item";
import _ from 'lodash'
import {CopyDocument} from "@element-plus/icons-vue";

export default {
  name: "table-inc-device",
  props: ['item', 'value', 'list'],
  model: {
    prop: 'value',
    event: 'input'
  },
  emits: ['createdDevice'],
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
  components: {CopyDocument, DlcItem},
  created() {
    this.$emit('createdDevice', {id: this.item.id, dlc: this.item.dlc});
    // console.log('created device id= '+ this.item.id)
  },
  methods:{
    copyToWriteTable(){
      this.emitter.emit("save-to-write-table", this.item);
    }
  }
}
</script>

<style scoped>

</style>