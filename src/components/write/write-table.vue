<template>
  <table class="table">
    <thead>
    <tr>
      <td>id</td>
      <td>count</td>
      <td>period</td>
      <td>dlc</td>
      <td colspan="8">data</td>
      <td>actions</td>
      <td>comment</td>
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
  <button @click="push">Добавить еще</button>
</template>

<script>

import WriteItem from "@/components/write/write-item";

export default {
  name: "write-table",
  components: {WriteItem},
  data() {
    return {
      outStock: [
        {
          id: "8EF87310",
          dlc: 2,
          count: 0,
          period: 0,
          data: ["F8", "0"],
          comment: ''
        }
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
      console.log('save');
      const data = JSON.stringify(this.outStock);
      const fs = require('fs');
      try {
        fs.writeFileSync('can-device-write.json', data, 'utf-8');
      } catch (e) {
        alert('Failed to save the file !');
      }
    },
    openFile: function () {
      const fs = require('fs');
      let jsonDB = fs.readFileSync('can-device-write.json', 'utf-8');

      this.outStock = JSON.parse(jsonDB);
    },
  }
}
</script>

<style scoped>

</style>