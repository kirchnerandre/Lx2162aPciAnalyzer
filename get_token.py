#!/usr/bin/env python3


import azure.identity
import azure.kusto.data


_cluster = "https://ingest-kvc-g17c54juuue55kc9ay.southcentralus.kusto.windows.net"


def main():
    credential = azure.identity.DeviceCodeCredential()

    token = credential.get_token("https://kusto.kusto.windows.net/.default")

    print(token.token)


if __name__ == "__main__":
    main()
