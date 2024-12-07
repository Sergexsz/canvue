/**
 * We'll load jQuery and the Bootstrap jQuery plugin which provides support
 * for JavaScript based Bootstrap features such as modals and tabs. This
 * code may be modified to fit the specific needs of your application.
 */

/**
 * Element PLUS
 */
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
// import './styles/element.scss'
import ru from 'element-plus/dist/locale/ru'
import 'dayjs/locale/ru'

import {createApp} from 'vue'
import App from './App.vue'
import {Quasar} from 'quasar'
import quasarUserOptions from './quasar-user-options'

createApp(App)
    .use(ElementPlus, {locale: ru})
    .use(Quasar, quasarUserOptions)
    .mount('#app')