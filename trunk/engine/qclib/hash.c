#include "qcc.h"
void Hash_InitTable(hashtable_t *table, int numbucks, void *mem)
{
	table->numbuckets = numbucks;
	table->bucket = (bucket_t **)mem;
}

int Hash_Key(char *name, int modulus)
{	//fixme: optimize.
	unsigned int key;
	for (key=0;*name; name++)
		key += ((key<<3) + (key>>28) + *name);
		
	return (int)(key%modulus);
}
int Hash_KeyInsensative(char *name, int modulus)
{	//fixme: optimize.
	unsigned int key;
	for (key=0;*name; name++)
	{
		if (*name >= 'A' && *name <= 'Z')
			key += ((key<<3) + (key>>28) + (*name-'A'+'a'));
		else
			key += ((key<<3) + (key>>28) + *name);
	}
		
	return (int)(key%modulus);
}

void *Hash_Get(hashtable_t *table, char *name)
{
	int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!STRCMP(name, buck->keystring))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetInsensative(hashtable_t *table, char *name)
{
	int bucknum = Hash_KeyInsensative(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!stricmp(name, buck->keystring))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetKey(hashtable_t *table, int key)
{
	int bucknum = key%table->numbuckets;
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if ((int)buck->keystring == key)
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetNext(hashtable_t *table, char *name, void *old)
{
	int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!STRCMP(name, buck->keystring))
		{
			if (buck->data == old)	//found the old one
				break;
		}

		buck = buck->next;
	}
	if (!buck)
		return NULL;

	buck = buck->next;//don't return old
	while(buck)
	{
		if (!STRCMP(name, buck->keystring))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetNextInsensative(hashtable_t *table, char *name, void *old)
{
	int bucknum = Hash_KeyInsensative(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!STRCMP(name, buck->keystring))
		{
			if (buck->data == old)	//found the old one
				break;
		}

		buck = buck->next;
	}
	if (!buck)
		return NULL;

	buck = buck->next;//don't return old
	while(buck)
	{
		if (!STRCMP(name, buck->keystring))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}


void *Hash_Add(hashtable_t *table, char *name, void *data, bucket_t *buck)
{
	int bucknum = Hash_Key(name, table->numbuckets);

	buck->data = data;
	buck->keystring = name;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}
void *Hash_AddInsensative(hashtable_t *table, char *name, void *data, bucket_t *buck)
{
	int bucknum = Hash_KeyInsensative(name, table->numbuckets);

	buck->data = data;
	buck->keystring = name;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}
#ifndef MINIMAL
void *Hash_AddKey(hashtable_t *table, int key, void *data)
{
	int bucknum = key%table->numbuckets;
	bucket_t *buck;	

	buck = qccHunkAlloc(sizeof(bucket_t));

	buck->data = data;
	buck->keystring = (char*)key;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}
#endif
void *Hash_AddKey2(hashtable_t *table, int key, void *data, bucket_t *buck)
{
	int bucknum = key%table->numbuckets;

	buck->data = data;
	buck->keystring = (char*)key;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}

void Hash_Remove(hashtable_t *table, char *name)
{
	int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;	

	buck = table->bucket[bucknum];

	if (!STRCMP(name, buck->keystring))
	{
		table->bucket[bucknum] = buck->next;
		return;
	}


	while(buck->next)
	{
		if (!STRCMP(name, buck->next->keystring))
		{
			buck->next = buck->next->next;
			return;
		}

		buck = buck->next;
	}
	return;
}

void Hash_RemoveData(hashtable_t *table, char *name, void *data)
{
	int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;	

	buck = table->bucket[bucknum];

	if (buck->data == data)
		if (!STRCMP(name, buck->keystring))
		{
			table->bucket[bucknum] = buck->next;
			return;
		}


	while(buck->next)
	{
		if (buck->next->data == data)
			if (!STRCMP(name, buck->next->keystring))
			{
				buck->next = buck->next->next;
				return;
			}

		buck = buck->next;
	}
	return;
}


void Hash_RemoveKey(hashtable_t *table, int key)
{
	int bucknum = key%table->numbuckets;
	bucket_t *buck;	

	buck = table->bucket[bucknum];

	if ((int)buck->keystring == key)
	{
		table->bucket[bucknum] = buck->next;
		return;
	}


	while(buck->next)
	{
		if ((int)buck->next->keystring == key)
		{
			buck->next = buck->next->next;
			return;
		}

		buck = buck->next;
	}
	return;
}
