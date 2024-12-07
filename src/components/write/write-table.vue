<template>
  <div class="bg-white p-2 shadow rounded mb-2">
    <div class="d-flex justify-content-between mb-2">
      <div class="fw-bold w-100 text-start">Список собственных пакетов</div>
      <button class="btn btn-success btn-sm" @click="push">Добавить</button>
    </div>
    <table class="table table-sm table-striped table-bordered">
      <thead>
      <tr class="tr fw-bold small text-muted text-uppercase sticky-top bg-white" style="top:0">
        <td style="width:120px">id</td>
        <td style="width:50px">dlc</td>
        <td colspan="8">data</td>
        <td>comment</td>
        <td style="width:80px">period</td>
        <td style="width:80px">count</td>
        <td>actions</td>
      </tr>
      </thead>
      <tbody>
      <write-item
          v-for="(out,index) in outStock"
          :key="index"
          v-model="outStock"
          :value="out"
          @input="saveFile"
          @delete="removeItem(out)"
          @written="writeMessage(out)"></write-item>
      </tbody>
    </table>
    <button class="btn btn-success w-100 my-1" @click="push">Добавить еще</button>
  </div>
</template>

<script>
import {ElMessage} from "element-plus";

const fs = require('fs');
import WriteItem from "./write-item"

export default {
  name: "write-table",
  components: {WriteItem},
  emits: ['written'],
  data() {
    return {
      outStock: [
        // {
        //   id: "8EF87310",
        //   dlc: 2,
        //   count: 0,
        //   period: 0,
        //   data: ["F8", "0"],
        //   comment: ''
        // }
      ]
    }
  },
  created() {
    this.openFile();
  },
  methods: {
    writeMessage(out) {
      let device = parseInt(out.id, 16);
      let dlc = out.dlc;
      let data = out.data.map(bit => {
        return parseInt(bit, 16);
      });
      this.$emit("written", 'w' + device + '.' + dlc + '.' + data.join('.'));
      out.count = +out.count + 1;
    },
    push() {
      this.outStock.push(
          {
            id: "",
            dlc: 0,
            count: 0,
            period: 0,
            data: [],
            comment: ''
          }
      );
      this.saveFile();
    },
    removeItem(out) {
      this.outStock.splice(this.outStock.indexOf(out), 1);
      this.saveFile();
    },

    saveFile: function () {
      const data = JSON.stringify(this.outStock);
      try {
        fs.writeFileSync('can-device-write.json', data, 'utf-8');
        ElMessage.success('Сохранено');
      } catch (e) {
        ElMessage.error('Не удалось сохранить в файл');
      }
    },
    openFile: function () {
      let jsonDB = fs.readFileSync('can-device-write.json', 'utf-8');
      this.outStock = JSON.parse(jsonDB);
    },


  }
}
</script>

<style scoped>

</style>