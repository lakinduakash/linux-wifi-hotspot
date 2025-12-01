<script lang="ts">
  import { onMount } from 'svelte';
  import Dashboard from './routes/Dashboard.svelte';
  import Login from './routes/Login.svelte';
  import { api } from '$lib/api';

  let token = localStorage.getItem('hotspotd_token') || '';
  let authenticated = false;

  onMount(async () => {
    if (token) {
      api.setToken(token);
      authenticated = true;
    }
  });

  function handleToken(event: CustomEvent<any>) {
    token = event.detail as string;
    api.setToken(token);
    localStorage.setItem('hotspotd_token', token);
    authenticated = true;
  }
</script>

{#if authenticated}
  <Dashboard />
{:else}
  <Login on:token={handleToken} />
{/if}
