import json
import logging
from telnetlib import NOP
from xmlrpc.client import Boolean
import boto3
import datetime
import os

logger = logging.getLogger()
logger.setLevel(logging.INFO)

NOTIFICATION_MIN_DELAY_MSECS = 10000

# TODO: add the decoding labels into DynamoDB and read them dinamically
def sensorCodeDecoder(code):

    mapper = {"PROX_1": "Living Room Window-1"
        , "PROX_2": "Living Room Window-2"
        , "PROX_3": "Main Door"
        , "PROX_4": "Study-Room Window"
        , "PROX_5": "Bedroom Window"
        , "PIR_1": "Living Room PIR"
        , "PIR_2": "Study-Room PIR"
        , "PIR_3": "Bedroom PIR"}
    
    if(code in mapper):
        return mapper[code]
    
    return "Unknown Sensor"

def printSensorStatus(s):
    out = ""
    out += "\t" + sensorCodeDecoder(s["deviceCode"])

    if(s["enabled"]):
        out += " [ENABLED]:"
    else:
        out += " [disabled]:"

    out += "\n\t\t - STATUS="
    
    if(s["alarm"]):
        out += "ALARM"
    else:
        out += "NORMAL"

    #out += "\n\t\t - VALUE=" + str(s["value"])

    out += "\n"

    return out

def manageArmedMode(event, context):

    logger.info("manageArmedMode - START")
    logger.info(str(event))

    if(event["status"] == None or event["status"]["alarm"] == None or event["status"]["alarmMode"] == None):
        logger.error("Invalid Event: " + str(event))
        return

    aws_account_id = context.invoked_function_arn.split(":")[4]
    aws_region = os.environ['AWS_REGION']

    dynamodb = boto3.resource("dynamodb", region_name=aws_region)
    sns = boto3.client('sns')

    statusTable=dynamodb.Table(os.environ['dynamoStatusTableName'])
    sensorHistTable=dynamodb.Table(os.environ['dynamoSensorHistoryTableName'])

    # TODO: add error handling
    response = statusTable.get_item(
        Key={'iotTopic': os.environ['iotTopicPub']}
    )

    now_datetime = datetime.datetime.now(tz=datetime.timezone.utc)
    now_ms = int(now_datetime.timestamp()*1000)

    dynamoRecord = type("MyRecord", (object,), {})()
    dynamoRecord.status = "UNKNOWN"
    dynamoRecord.lastAlarm = 0

    # get latest status from DynamoDB
    if('Item' in response):
        dynamoRecord.lastAlarm = response['Item']['lastAlarmNotificationTmsMs']
        dynamoRecord.status = response['Item']['alarmStatus']
        dynamoRecord.mode = response['Item']['alarmMode']
        dynamoRecord.sensors = response['Item']['sensors']

    # just keep "enabled" and "deviceCode" attributes, remove the others
    for i in range(len(dynamoRecord.sensors)):
        dynamoRecord.sensors[i]=dict((k,v) for k,v in dynamoRecord.sensors[i].items() if k in ('enabled', 'deviceCode'))

    # if the "ALARM" flag sent by the IOT device is equal to true => newStatus="ALARM", otherwise newStatus = "NORMAL"
    incomingIotStatus = ("NORMAL", "ALARM")[event["status"]["alarm"]]
    #incomingIotEvent:Boolean = event["status"]["changeStatusEvent"]

    logger.info("incomingIotStatus: " + incomingIotStatus)
    logger.info("dynamoRecord.status: " + dynamoRecord.status)

    """
    There are 3 alternative conditions for sending messages to SNS:
        1) If the STATUS read from the IOT device is different from the one detected on DynamoDB
        2) If the IOT device has been ARMED
        3) If the alarm is ARMED and the minimim amount of delay passed
    """
    if((incomingIotStatus != dynamoRecord.status) 
        or dynamoRecord.mode != event["status"]["alarmMode"]
        or (event["status"]["alarm"] and ((now_ms-dynamoRecord.lastAlarm)>NOTIFICATION_MIN_DELAY_MSECS))):

        filterAlarmed=lambda x:x["alarm"]
        alarmedSensors = list(filter(filterAlarmed, event["sensors"]))

        message_text = "Alarm Status = " + incomingIotStatus + "\n\n"

        if(incomingIotStatus == "ALARM"):
            snsSubject = "Alarm Event on " + os.environ['iotTopicPub']
            message_text += "ALARMED SENSORS\n\n" + '\n'.join(list(map(lambda x:printSensorStatus(x), alarmedSensors))) + "\n\n"
    
        # Some relevant event from the IOT device has been detected (eg: ARM/DISARM)
        #elif(incomingIotEvent):
        #    snsSubject = "Event detected on the Plant"
        
        # nothing special happened
        else:
            snsSubject = "Normal Situation on " + os.environ['iotTopicPub']
            message_text += "Sensors Overall Status\n\n" + '\n'.join(list(map(lambda x:printSensorStatus(x), list(event["sensors"]) )))

        # Publish the formatted message
        response = sns.publish(
            TopicArn = 'arn:aws:sns:' + aws_region + ':' + aws_account_id + ':' + os.environ['snsTopic'],
            Subject = snsSubject,
            Message = message_text
        )

        if(event["status"]["alarm"]):
            statusTable.update_item(
                Key={"iotTopic": os.environ['iotTopicPub']},
                UpdateExpression="set alarmStatus=:s, lastAlarmNotificationTmsMs=:a, lastUpdateTmsMs=:u, sensors=:as, alarmMode=:md",
                ExpressionAttributeValues={":s": incomingIotStatus, ":a": now_ms, ":u": now_ms, ":as": event["sensors"], ":md": event["status"]["alarmMode"]}, ReturnValues="UPDATED_NEW"
            )
        else:
                statusTable.update_item(
                Key={"iotTopic": os.environ['iotTopicPub']},
                UpdateExpression="set alarmStatus=:s, lastUpdateTmsMs=:u, sensors=:as, alarmMode=:md",
                ExpressionAttributeValues={":s": incomingIotStatus, ":u": now_ms, ":as": event["sensors"], ":md": event["status"]["alarmMode"]}, ReturnValues="UPDATED_NEW"
            )
    else:
        statusTable.update_item(
            Key={"iotTopic": os.environ['iotTopicPub']},
            UpdateExpression="set lastUpdateTmsMs=:u, sensors=:as, alarmMode=:md",
            ExpressionAttributeValues={":u": now_ms, ":as": event["sensors"], ":md": event["status"]["alarmMode"]}, ReturnValues="UPDATED_NEW"
        )
    
    # TODO: add error handling
    with sensorHistTable.batch_writer() as batch:
        for item in event["sensors"]:
            batch.put_item(Item={
                "id": item["deviceCode"],
                "datetime": now_datetime.strftime("%Y%m%d%H%M%S"),
                "alarm": item["alarm"],
                "enabled": item["enabled"],
                "lastCheck": item["lastCheck"],
                "value": item["value"]
            })

    return response

# Experimental - to be tested
def manageMonitorMode(event, context):

    logger.info("manageMonitorMode - START")
    logger.info(str(event))

    if(event["status"] == None or event["status"]["alarm"] == None or event["status"]["alarmMode"] == None):
        logger.error("Invalid Event: " + str(event))
        return

    aws_region = os.environ['AWS_REGION']
    dynamodb = boto3.resource("dynamodb", region_name=aws_region)
    statusTable=dynamodb.Table(os.environ['dynamoStatusTableName'])
    sensorHistTable=dynamodb.Table(os.environ['dynamoSensorHistoryTableName'])

    now_datetime = datetime.datetime.now(tz=datetime.timezone.utc);
    now_ms = int(now_datetime.timestamp()*1000)

    # if the "ALARM" flag sent by the IOT device is equal to true => newStatus="ALARM", otherwise newStatus = "NORMAL"
    incomingIotStatus = ("NORMAL", "ALARM")[event["status"]["alarm"]]
    #incomingIotEvent:Boolean = event["status"]["changeStatusEvent"]

    statusTable.update_item(
        Key={"iotTopic": os.environ['iotTopicPub']},
        UpdateExpression="set alarmStatus=:s, lastUpdateTmsMs=:u, sensors=:as, alarmMode=:md",
        ExpressionAttributeValues={":s": incomingIotStatus, ":u": now_ms, ":as": event["sensors"], ":md": event["status"]["alarmMode"]}, ReturnValues="UPDATED_NEW"
    )

    # TODO: add error handling
    with sensorHistTable.batch_writer() as batch:
        for item in event["sensors"]:
            batch.put_item(Item={
                "id": item["deviceCode"],
                "datetime": now_datetime.strftime("%Y%m%d%H%M%S"),
                "alarm": item["alarm"],
                "enabled": item["enabled"],
                "lastCheck": item["lastCheck"],
                "value": item["value"]
            })

    return

def manageSyncMode(event, context):

    logger.info("manageSyncMode - START")
    logger.info(str(event))

    if(event["status"] == None or event["status"]["alarm"] == None or event["status"]["alarmMode"] == None):
        logger.error("Invalid Event: " + str(event))
        return

    aws_account_id = context.invoked_function_arn.split(":")[4]
    aws_region = os.environ['AWS_REGION']

    dynamodb = boto3.resource("dynamodb", region_name=aws_region)
    sns = boto3.client('sns')

    statusTable=dynamodb.Table(os.environ['dynamoStatusTableName'])

    # TODO: add error handling
    response = statusTable.get_item(
        Key={'iotTopic': os.environ['iotTopicPub']}
    )

    dynamoRecord = type("MyRecord", (object,), {})()
    dynamoRecord.status = "UNKNOWN"
    dynamoRecord.lastAlarm = 0

    # get latest status from DynamoDB
    if('Item' in response):
        dynamoRecord.lastAlarm = response['Item']['lastAlarmNotificationTmsMs']
        dynamoRecord.status = response['Item']['alarmStatus']
        dynamoRecord.mode = response['Item']['alarmMode']
        dynamoRecord.sensors = response['Item']['sensors']

    # just keep "enabled" and "deviceCode" attributes, remove the others
    for i in range(len(dynamoRecord.sensors)):
        dynamoRecord.sensors[i]=dict((k,v) for k,v in dynamoRecord.sensors[i].items() if k in ('enabled', 'deviceCode'))
    
    response = sns.publish(
        TopicArn = 'arn:aws:sns:' + aws_region + ':' + aws_account_id + ':' + os.environ['snsTopic'],
        Subject = "Sync request from Iot Device",
        Message = "Sync request from Iot Device"
    )

    session = boto3.Session(region_name=aws_region)
    iot_client = session.client("iot")

    # use the ATS endpoint to avoid SSL issues as documented here: https://github.com/boto/boto3/issues/2686
    iotClient = session.client('iot-data', endpoint_url=f'https://{iot_client.describe_endpoint(endpointType="iot:Data-ATS").get("endpointAddress")}')

    response = iotClient.publish(
        topic=os.environ['iotTopicSub'],
        qos=1,
        payload=json.dumps(
            {
                "commands": {"alarmMode":dynamoRecord.mode},
                "sensors": dynamoRecord.sensors
            }
        )
    )
    logger.info("MQTT SYNC message has been sent");

    return response

def manageOffMode(event, context):

    logger.info("manageOffMode - START")
    logger.info(str(event))

    if(event["status"] == None or event["status"]["alarm"] == None or event["status"]["alarmMode"] == None):
        logger.error("Invalid Event: " + str(event))
        return

    aws_region = os.environ['AWS_REGION']
    dynamodb = boto3.resource("dynamodb", region_name=aws_region)
    statusTable=dynamodb.Table(os.environ['dynamoStatusTableName'])
    sensorHistTable=dynamodb.Table(os.environ['dynamoSensorHistoryTableName'])

    # TODO: add error handling
    response = statusTable.get_item(
        Key={'iotTopic': os.environ['iotTopicPub']}
    )

    dynamoRecord = type("MyRecord", (object,), {})()

    # get latest status from DynamoDB
    if('Item' in response):
        dynamoRecord.lastAlarm = response['Item']['lastAlarmNotificationTmsMs']
        dynamoRecord.mode = response['Item']['alarmMode']

    now_datetime = datetime.datetime.now(tz=datetime.timezone.utc);
    now_ms = int(now_datetime.timestamp()*1000)
    aws_account_id = context.invoked_function_arn.split(":")[4]

    # if the "ALARM" flag sent by the IOT device is equal to true => newStatus="ALARM", otherwise newStatus = "NORMAL"
    incomingIotStatus = ("NORMAL", "ALARM")[event["status"]["alarm"]]

    # If NOT already in OFF MODE => publish the formatted message
    if(dynamoRecord.mode != event["status"]["alarmMode"]):
        sns = boto3.client('sns')
        snsSubject = "Device Mode OFF on " + os.environ['iotTopicPub']
        message_text = "Device Mode OFF \n\n"
        message_text += "Sensors Overall Status\n\n" + '\n'.join(list(map(lambda x:printSensorStatus(x), list(event["sensors"]))))
        response = sns.publish(
            TopicArn = 'arn:aws:sns:' + aws_region + ':' + aws_account_id + ':' + os.environ['snsTopic'],
            Subject = snsSubject,
            Message = message_text
        )
    
    # in any case, update the DynamoDB Table
    statusTable.update_item(
        Key={"iotTopic": os.environ['iotTopicPub']},
        UpdateExpression="set alarmStatus=:s, lastUpdateTmsMs=:u, sensors=:as, alarmMode=:md",
        ExpressionAttributeValues={":s": incomingIotStatus, ":u": now_ms, ":as": event["sensors"], ":md": event["status"]["alarmMode"]}, ReturnValues="UPDATED_NEW"
    )

    # TODO: add error handling
    with sensorHistTable.batch_writer() as batch:
        for item in event["sensors"]:
            batch.put_item(Item={
                "id": item["deviceCode"],
                "datetime": now_datetime.strftime("%Y%m%d%H%M%S"),
                "alarm": item["alarm"],
                "enabled": item["enabled"],
                "lastCheck": item["lastCheck"],
                "value": item["value"]
            })

    return response

    # Use this code if you don't use the http event with the LAMBDA-PROXY
    # integration
"""
    return {
        "message": "Go Serverless v1.0! Your function executed successfully!",
        "event": event
    }
    """