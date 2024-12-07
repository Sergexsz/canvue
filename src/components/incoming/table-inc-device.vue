<template>
  <tr>
    <td>{{ device.id }}</td>
    <td>{{ device.count }}</td>
    <td>± {{ device.frequency }}мс</td>
    <td>{{ device.dlc }}</td>
    <!--          data-->
    <dlc-item
        v-for="n in data_lenght" :key="n"
        :data="device.data[n-1]"></dlc-item>
    <!--          comment-->
    <td>
      <label>
        <!--        <textarea @blur="saveFile" style="height: 20px" class="form-control" v-model="device.comment"></textarea>-->
      </label>
    </td>

  </tr>
</template>

<script>
import DlcItem from "@/components/dlc-item";

export default {
  name: "table-inc-device",
  props: ['device'],
  data() {
    return {
      data_lenght: 8
    }
  },
  components: {DlcItem},
}
</script>

<style scoped>

</style>