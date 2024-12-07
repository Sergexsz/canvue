<template>
  <h4 class="h5 w-100 text-start">Список собственных пакетов</h4>
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
    <template :key="index" v-for="(out,index) in outStock">
      <write-item v-model="outStock" :value="out"
                  @delete="removeItem(out)"
                  @input="saveFile"
                  @write="writeMessage(out)"></write-item>
    </template>
    </tbody>
  </table>
  <button class="btn btn-success w-100 my-1" @click="push">Добавить еще</button>
</template>

<script>
const fs = require('fs');
import WriteItem from "./write-item"

export default {
  name: "write-table",
  components: {WriteItem},
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
      this.$emit("write", 'w' + device + '.' + dlc + '.' + data.join('.'));
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
      } catch (e) {
        alert('Failed to save the file !');
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