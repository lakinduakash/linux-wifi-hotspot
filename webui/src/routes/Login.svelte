<script lang="ts">
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
  let token = '';

  function submit() {
    dispatch('token', token);
  }
</script>

<div class="login">
  <h1>Connect to Hotspot</h1>
  <input
    type="password"
    placeholder="API Token"
    bind:value={token}
    on:keydown={(e) => e.key === 'Enter' && submit()}
  />
  <button on:click={submit}>Save</button>
</div>

<style>
  .login {
    max-width: 360px;
    margin: 20vh auto;
    background: #1f2937;
    padding: 1.5rem;
    border-radius: 12px;
    box-shadow: 0 15px 35px rgba(0,0,0,0.4);
  }
  input {
    width: 100%;
    padding: 0.75rem;
    border-radius: 8px;
    border: none;
    margin-bottom: 1rem;
    background: #111827;
    color: #f3f4f6;
  }
  button {
    width: 100%;
    padding: 0.75rem;
    background: #2563eb;
    border: none;
    border-radius: 8px;
    color: #fff;
    cursor: pointer;
  }
</style>
