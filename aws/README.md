# HAP - AWS Setup
Here you'll find the resources for setting up the AWS Services needed for managing the Home Alarm Project.

1. Edit the \*json files replacing the following parameters:
   - 01.sns-params.json:
      - *SubscriptionEmail*\
        it's the email address you want to be subscribed to the SNS topic for notifications
   - 02.iot-params.json:
      - *IoTCertName*\
        it's the THING certification name you should have from previous steps (see [prerequirements](../README.md#aws-setup) )
    - 04.rest-api-params.json:
      - *IOTSubdomain*\
        it's the IOT Subdomain. You can get it running the following command:
        > aws iot describe-endpoint --endpoint-type iot:Data-ATS --output text | cut -d "-" -f 1
2. Setup your preferred profile:
  > AWS_PROFILE=\<MY_PROFILE\>
3. Create the SNS Stack:
  > aws --profile $AWS_PROFILE cloudformation create-stack --stack-name simple-home-alarm-resources-sns --template-body file://01.sns.yaml --parameters file://./01.sns-params.json
4. Create the IOT Stack:
  > aws --profile $AWS_PROFILE cloudformation create-stack --stack-name simple-home-alarm-resources-iot --template-body file://02.iot.yaml --parameters file://./02.iot-params.json
5. Setup Cognito user parameters:
> USERNAME=\<MY_USERNAME\>
> 
> PASSWD=\<MY_PASSW\>
> 
> REGION=\<MY_REGION\>

6. Create the Cognito Stack
  > aws --profile $AWS_PROFILE cloudformation create-stack --stack-name simple-home-alarm-resources-cognito --template-body file://03.cognito.yaml --parameters file://./03.cognito-params.json --capabilities CAPABILITY_NAMED_IAM
7. Get back some useful parameters and create the AndroidApp User: 
  >  UPID=$(aws --profile $AWS_PROFILE cognito-idp list-user-pools --query "UserPools[?Name == 'simpleCognito-user-pool'].{Id:Id}" --max-results 10 --output text)
  >  
  >  UPCID=$(aws --profile $AWS_PROFILE cognito-idp list-user-pool-clients --user-pool-id $UPID --query "UserPoolClients[?ClientName == 'simpleCognito-client'].{Id:ClientId}" --max-results 10 --output text)
  >  
  >  IPID=$(aws --profile $AWS_PROFILE cognito-identity list-identity-pools --max-results 10 --query "IdentityPools[?IdentityPoolName == 'simpleCognito-identity-pool'].{Id:IdentityPoolId}" --output text)
  >  
  >  aws --profile $AWS_PROFILE cognito-idp admin-create-user --user-pool-id $UPID --username $USERNAME --temporary-password "$PASSWD"
  >  
  >  aws --profile $AWS_PROFILE cognito-idp admin-set-user-password --user-pool-id $UPID --username $USERNAME --password "$PASSWD" --permanent
  >  
  >  aws cognito-idp initiate-auth --region $REGION --auth-flow USER_PASSWORD_AUTH --client-id $UPCID --auth-parameters USERNAME=$USERNAME,PASSWORD="$PASSWD"

8. Create the API-Gateway Stack:
  > aws --profile $AWS_PROFILE cloudformation create-stack --stack-name simple-home-alarm-resources-restapi --template-body file://04.rest-api.yaml --parameters file://./04.rest-api-params.json --capabilities CAPABILITY_NAMED_IAM

9. Create the empty dummy record into the DynamoDB Table
  > aws --profile $AWS_PROFILE dynamodb put-item \\
  > 
  > --table-name simpleAlarmStatusTable \\
  > 
  > --item \\
  > 
  > '{"iotTopic": {"S":"esp32/pub"},"lastAlarmNotificationTmsMs":{"N":"1663512656619"}, "alarmStatus":{"S": ""}, "alarmMode":{"S": "OFF"},"sensors":{"L":[{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_1"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_2"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_3"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_4"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_5"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"71"},"deviceCode":{"S":"PIR_1"},"value":{"S":"2099.9433"},"enabled":{"BOOL":true}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PIR_2"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PIR_3"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}}]}, "lastUpdateTmsMs":{"N":"1663512656619"}}'

10. Deploy the Lambda handler using Serverless framework:
 > sls deploy --aws-profile $AWS_PROFILE 