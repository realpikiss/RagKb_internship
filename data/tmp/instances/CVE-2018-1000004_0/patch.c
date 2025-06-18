static struct snd_seq_client *seq_create_client1(int client_index, int poolsize)
{
	unsigned long flags;
	int c;
	struct snd_seq_client *client;

	/* init client data */
	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (client == NULL)
		return NULL;
	client->pool = snd_seq_pool_new(poolsize);
	if (client->pool == NULL) {
		kfree(client);
		return NULL;
	}
	client->type = NO_CLIENT;
	snd_use_lock_init(&client->use_lock);
	rwlock_init(&client->ports_lock);
	mutex_init(&client->ports_mutex);
	INIT_LIST_HEAD(&client->ports_list_head);
	mutex_init(&client->ioctl_mutex);

	/* find free slot in the client table */
	spin_lock_irqsave(&clients_lock, flags);
	if (client_index < 0) {
		for (c = SNDRV_SEQ_DYNAMIC_CLIENTS_BEGIN;
		     c < SNDRV_SEQ_MAX_CLIENTS;
		     c++) {
			if (clienttab[c] || clienttablock[c])
				continue;
			clienttab[client->number = c] = client;
			spin_unlock_irqrestore(&clients_lock, flags);
			return client;
		}
	} else {
		if (clienttab[client_index] == NULL && !clienttablock[client_index]) {
			clienttab[client->number = client_index] = client;
			spin_unlock_irqrestore(&clients_lock, flags);
			return client;
		}
	}
	spin_unlock_irqrestore(&clients_lock, flags);
	snd_seq_pool_delete(&client->pool);
	kfree(client);
	return NULL;	/* no free slot found or busy, return failure code */
}