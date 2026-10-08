#!/usr/bin/env python3

#import azure.kusto.data
#import azure.kusto.data.data_format
#import azure.kusto.ingest
import datetime
import json
import os
import pathlib
import sys
import time


_cluster    = "https://ingest-kvc-g17c54juuue55kc9ay.southcentralus.kusto.windows.net"
_database   = "kirchnerandre-database"
_table      = "Lx2162aPciAnalyzer"


def read_Lx2162aPciAnalyzer_data(FilePath, Debug):
    try:
        data = open(FilePath, "r").read()

        if data != "":
            version = int(data.readline())
            errors  = int(data.readline())

            for i in range(errors):
                values = data.readline().split(".")

                timestamp_up                            = int(values[0])
                timestamp_down                          = int(values[1])
                uncorrectable_error_status_register     = int(values[2])
                uncorrectable_error_severity_register   = int(values[3])
                correctable_error_status_register       = int(values[4])
                header_log_register_dword_1             = int(values[5])
                header_log_register_dword_2             = int(values[6])
                header_log_register_dword_3             = int(values[7])
                header_log_register_dword_4             = int(values[8])
                root_error_status_register              = int(values[9])
                correctable_error_source_id_register    = int(values[10])
                error_source_id_register                = int(values[11])
                lane_error_status_register              = int(values[12])

                if Debug:
                    kcsb        = azure.kusto.data.KustoConnectionStringBuilder.with_az_cli_authentication(_cluster)
                    client      = azure.kusto.ingest.QueuedIngestClient(kcsb)

                    row = [
                        TimestampUp,
                        TimestampDown,
                        UncorrectableErrorStatusRegister,
                        UncorrectableErrorSeverityRegister,
                        CorrectableErrorStatusRegister,
                        HeaderLogRegisterDword1,
                        HeaderLogRegisterDword2,
                        HeaderLogRegisterDword3,
                        HeaderLogRegisterDword4,
                        RootErrorStatusRegister,
                        CorrectableErrorSourceIdRegister,
                        ErrorSourceIdRegister,
                        LaneErrorStatusRegister
                    ]

                    csv_buffer = io.StringIO()

                    csv.writer(csv_buffer).writerow(row)

                    stream = io.BytesIO(csv_buffer.getvalue().encode("utf-8"))

                    properties = azure.kusto.ingest.IngestionProperties(database=_database, table=_table, data_format=azure.kusto.data.data_format.DataFormat.CSV,)

                    client.ingest_from_stream(azure.kusto.ingest.StreamDescriptor(stream), ingestion_properties=properties,)
                else:
                    print(
                        f"{timestamp_up}."
                        f"{timestamp_down}."
                        f"{uncorrectable_error_status_register:08X}."
                        f"{uncorrectable_error_severity_register:08X}."
                        f"{correctable_error_status_register:08X}."
                        f"{header_log_register_dword_1:08X}."
                        f"{header_log_register_dword_2:08X}."
                        f"{header_log_register_dword_3:08X}."
                        f"{header_log_register_dword_4:08X}."
                        f"{root_error_status_register:08X}."
                        f"{correctable_error_source_id_register:08X}."
                        f"{error_source_id_register:08X}."
                        f"{lane_error_status_register:08X}")

        return True
    except Exception as e:
        print(e)
        return False


def main(Debug):
    file_path = pathlib.Path("/dev/Lx2162aPciAnalyzer")

    if read_Lx2162aPciAnalyzer_data(file_path, Debug) == False:
        print("Failed to read /dev/Lx2162aPciAnalyzer data")
        return -1

    return 0


if __name__ == "__main__":
    if "--debug" in sys.argv:
        main(False)
    else:
        main(True)
