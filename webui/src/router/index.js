import { createRouter, createWebHashHistory } from 'vue-router'
import HomeView from '@/views/HomeView.vue'

const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    { path: '/', name: 'home', component: HomeView },
    { path: '/charger', name: 'charger', component: () => import('@/views/ChargerView.vue') },
    { path: '/games', name: 'games', component: () => import('@/views/GamesView.vue') },
    { path: '/logs', name: 'logs', component: () => import('@/views/LogsView.vue') },
    { path: '/about', name: 'about', component: () => import('@/views/AboutView.vue') },
    { path: '/:pathMatch(.*)*', redirect: '/' }
  ]
})

export default router
