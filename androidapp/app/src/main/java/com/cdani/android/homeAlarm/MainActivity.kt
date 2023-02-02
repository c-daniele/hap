package com.cdani.android.homeAlarm

//import com.amazonaws.mobileconnectors.iot.AWSIotMqttManager

import android.app.AlertDialog
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import com.amplifyframework.api.ApiException
import com.amplifyframework.api.rest.RestOperation
import com.amplifyframework.api.rest.RestOptions
import com.amplifyframework.api.rest.RestResponse
import com.amplifyframework.core.Amplify
import org.json.JSONArray
import org.json.JSONException
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.time.Instant
import java.time.LocalDateTime
import java.time.ZoneId
import java.time.ZoneOffset
import java.time.format.DateTimeFormatter
import java.util.*


/*import com.amazonaws.mobileconnectors.iot.AWSIotMqttQos;
import com.amazonaws.mobileconnectors.iot.AWSIotMqttNewMessageCallback;*/

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)


        setContentView(R.layout.activity_main)
    }

    fun onUpdateBtn(view: View){

        Log.d("MainActivity::onUpdateBtn", "START");

        val radioControl = findViewById<RadioGroup>(R.id.radioControl);
        val radioButtonID: Int = radioControl.checkedRadioButtonId;
        val radioButton = findViewById<RadioButton>(radioButtonID);
        val ckboxLivingWindow1 = findViewById<CheckBox>(R.id.checkBox1);
        val ckboxLivingWindow2 = findViewById<CheckBox>(R.id.checkBox2);
        val ckboxMainDoor = findViewById<CheckBox>(R.id.checkBox3);
        val ckboxStudyroomWindow = findViewById<CheckBox>(R.id.checkBox4);
        val ckboxBedroomWindow = findViewById<CheckBox>(R.id.checkBox5);
        val ckboxLivingroomPir = findViewById<CheckBox>(R.id.checkBox6);
        val ckboxStudyroomPir = findViewById<CheckBox>(R.id.checkBox7);
        val ckboxBedroomPir = findViewById<CheckBox>(R.id.checkBox8);

        val mode = radioButton.text.toString().uppercase();

        val cmdBuilder = StringBuilder()

        cmdBuilder.append("{\"commands\": {\"alarmMode\": ")
            .append("\"" + mode + "\"")
            .append("},\"sensors\": [")
            .append("{\"deviceCode\":\"").append(ckboxLivingWindow1.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxLivingWindow1.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxLivingWindow2.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxLivingWindow2.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxMainDoor.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxMainDoor.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxStudyroomWindow.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxStudyroomWindow.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxBedroomWindow.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxBedroomWindow.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxLivingroomPir.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxLivingroomPir.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxStudyroomPir.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxStudyroomPir.isChecked()) "true" else "false").append("}")
            .append(",{\"deviceCode\":\"").append(ckboxBedroomPir.contentDescription).append("\"")
            .append(",\"enabled\":").append(if(ckboxBedroomPir.isChecked()) "true" else "false").append("}")
            .append("]}");

        val cmd:String = cmdBuilder.toString();

        Log.i("MainActivity::onUpdateBtn", "cmd: $cmd");

        /*
        // TO BE USED FOR TOKEN BASED-API-AUTHORIZER
        var token:String? = "";
        Amplify.Auth.fetchAuthSession(
            { result: AuthSession ->
                val cognitoAuthSession = result as AWSCognitoAuthSession
                if (cognitoAuthSession.userPoolTokens
                        .type == AuthSessionResult.Type.FAILURE
                ) {
                    // Handling no session here.
                }
                assert(cognitoAuthSession.userPoolTokens != null);
                assert(cognitoAuthSession.userPoolTokens.value != null);
                token = cognitoAuthSession?.userPoolTokens?.value?.idToken;
                Log.i("MyAmplifyApp", "ID-TOKEN: $token");
            }
        ) { error: AuthException ->
            Log.e(
                "AuthQuickStart",
                error.toString()
            )
        }

        //TODO: improve sync with token callback
        while("" == token){
            Log.i("MyAmplifyApp", "sleeping...");
            Thread.sleep(500);
        }
        Log.i("MyAmplifyApp", "token ok: $token");
        */

        val options = RestOptions.builder()
            .addPath("/homealarmcommand")
            .addHeader("Content-Type", "application/json")
            //.addHeader("Authorization", token) // TO BE USED FOR TOKEN BASED-API-AUTHORIZER
            .addBody(cmd.toByteArray())
            .build();

        Log.i("MyAmplifyApp", "request: $options");

        Amplify.API.post(options,
            {
                Log.i("MyAmplifyApp", "POST succeeded")
                Log.i("MyAmplifyApp", "Response Code: " + it.code)
                Log.i("MyAmplifyApp", "Response Header: " + it.headers)
                Log.i("MyAmplifyApp", "Response Body: " + it.data.asString())
            },
            { Log.e("MyAmplifyApp", "POST failed", it) }
        )

    }

    fun onSyncBtn(view: View){

        Log.d("MainActivity::onSyncBtn", "START");

        val refreshButton:Button = findViewById<Button>(R.id.button3);
        val progressBar1:ProgressBar = findViewById<ProgressBar>(R.id.progressBar);
        //refreshButton.isEnabled = false

        // temporary disable UX components
        refreshButton.visibility = View.GONE;
        progressBar1.visibility = View.VISIBLE;
        findViewById<CheckBox>(R.id.checkBox1).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox2).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox3).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox4).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox5).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox6).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox7).isEnabled = false;
        findViewById<CheckBox>(R.id.checkBox8).isEnabled = false;
        findViewById<Button>(R.id.button2).isEnabled = false;
        findViewById<RadioButton>(R.id.radioArmed).isEnabled = false;
        findViewById<RadioButton>(R.id.radioOff).isEnabled = false;
        findViewById<RadioButton>(R.id.radioLog).isEnabled = false;

        val bodyBuilder = StringBuilder();
        bodyBuilder.append("{\"TableName\": \"simpleAlarmStatusTable\"")
            .append(", \"Key\": {\"iotTopic\": {\"S\":\"esp32/pub\"}}")
            .append(", \"ProjectionExpression\": \"alarmMode, lastAlarmNotificationTmsMs, lastUpdateTmsMs, sensors\"")
            .append(", \"ReturnConsumedCapacity\": \"NONE\"}");

        val body:String = bodyBuilder.toString();

        Log.i("MainActivity::onSyncBtn", "cmd: $body");

        /*
        // TO BE USED FOR TOKEN BASED-API-AUTHORIZER
        var token:String? = "";
        Amplify.Auth.fetchAuthSession(
            { result: AuthSession ->
                val cognitoAuthSession = result as AWSCognitoAuthSession
                if (cognitoAuthSession.userPoolTokens
                        .type == AuthSessionResult.Type.FAILURE
                ) {
                    // Handling no session here.
                }
                assert(cognitoAuthSession.userPoolTokens != null);
                assert(cognitoAuthSession.userPoolTokens.value != null);
                token = cognitoAuthSession?.userPoolTokens?.value?.idToken;
                Log.i("MyAmplifyApp", "ID-TOKEN: $token");
            }
        ) { error: AuthException ->
            Log.e(
                "AuthQuickStart",
                error.toString()
            )
        }

        //TODO: improve sync with token callback
        while("" == token){
            Log.i("MyAmplifyApp", "sleeping...");
            Thread.sleep(500);
        }
        Log.i("MyAmplifyApp", "token ok: $token");
        */

        val options = RestOptions.builder()
            .addPath("/homealarmstatus")
            .addHeader("Content-Type", "application/json")
            //.addHeader("Authorization", token) // TO BE USED FOR TOKEN BASED-API-AUTHORIZER
            .addBody(body.toByteArray())
            .build();

        Log.i("MyAmplifyApp", "request: $options");

        val retVal: RestOperation? = Amplify.API.post(options
            , this::getStatusSuccessCallback
            , this::getStatusErrorCallback
        );

        Log.d("MainActivity::onSyncBtn", "END");

    }

    private fun getStatusSuccessCallback(it: RestResponse){

        Log.i("MyAmplifyApp", "POST succeeded")
        Log.i("MyAmplifyApp", "Response Code: " + it.code)
        Log.i("MyAmplifyApp", "Response Header: " + it.headers)
        Log.i("MyAmplifyApp", "Response Body: " + it.data.asString())

        val jsonObject:JSONObject = it.data.asJSONObject();

        var item:JSONObject? = null;
        item = try{
            jsonObject.getJSONObject("Item");
        } catch (error: JSONException) {

            Log.e("HomeAlarmApp", "JSON Exception", error);

            this@MainActivity.runOnUiThread(java.lang.Runnable {

                val now:String = DateTimeFormatter
                    .ofPattern("dd/MM/yyyy HH:mm:ss")
                    .withZone(ZoneOffset.UTC)
                    .format(Instant.now());
                val refreshButton:Button = findViewById<Button>(R.id.button3);
                val progressBar1:ProgressBar = findViewById<ProgressBar>(R.id.progressBar);
                val lastUpd:TextView = findViewById<TextView>(R.id.textView2);

                //refreshButton.isEnabled = false
                refreshButton.visibility = View.VISIBLE;
                progressBar1.visibility = View.GONE;
                lastUpd.text = now;

                val builder = AlertDialog.Builder(this)
                builder.setTitle("Error")
                builder.setMessage(error.localizedMessage)

                builder.setNeutralButton("OK") { dialog, which ->
                    Toast.makeText(applicationContext,
                        "ok", Toast.LENGTH_SHORT).show()
                }
                builder.show()

            })

            null;
        }

        /*
        Response Example
        {"Item": {
            "lastAlarmNotificationTmsMs":{"N":"1663512656619"}
            ,"alarmStatus": {"S": "NORMAL"},
            ,"alarmMode": {"S": "OFF"},
            ,"sensors":{"L":[
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_1"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_2"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_3"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_4"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_5"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"71"},"deviceCode":{"S":"PIR_1"},"value":{"S":"2099.9433"},"enabled":{"BOOL":true}}}
                ,{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PIR_2"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}}
                ,{"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PIR_3"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}}]}
            , "lastUpdateTmsMs":{"N":"1663512656619"}
            }
        }
         */

        if(item != null) {
            this@MainActivity.runOnUiThread(java.lang.Runnable {

                val now: String = DateTimeFormatter
                    .ofPattern("dd/MM/yyyy HH:mm:ss")
                    .withZone(ZoneId.of("Europe/Rome").rules.getOffset(LocalDateTime.now()))
                    .format(Instant.now());
                val refreshButton: Button = findViewById<Button>(R.id.button3);
                val progressBar1: ProgressBar = findViewById<ProgressBar>(R.id.progressBar);
                val lastUpd: TextView = findViewById<TextView>(R.id.textView2);
                val radioArmed = findViewById<RadioButton>(R.id.radioArmed);
                val radioLog = findViewById<RadioButton>(R.id.radioLog);
                val radioOff = findViewById<RadioButton>(R.id.radioOff);

                //refreshButton.isEnabled = false
                refreshButton.visibility = View.VISIBLE;
                progressBar1.visibility = View.GONE;
                lastUpd.text = now;
                findViewById<Button>(R.id.button2).isEnabled = true;

                // set ARMED switch button
                radioArmed.isEnabled = true;
                radioLog.isEnabled = true;
                radioOff.isEnabled = true;
                try{
                    when (item.getJSONObject("alarmMode").getString("S")) {
                        "ARMED" -> radioArmed.isChecked = true
                        "MONITOR" -> radioLog.isChecked = true
                        "OFF" -> radioOff.isChecked = true
                    }
                }catch (error: JSONException) {
                    Log.e("HomeAlarmApp", "JSON Exception", error);
                    radioOff.isChecked = true
                }

                // Last Msg Received
                findViewById<TextView>(R.id.textView6).text = try{
                    SimpleDateFormat("dd/MM/yyyy HH:mm:ss").format(Date(item.getJSONObject("lastUpdateTmsMs").getLong("N")))
                }catch (error: JSONException) {
                    Log.e("HomeAlarmApp", "JSON Exception", error);
                    "ERROR";
                }

                // Last Alarm
                findViewById<TextView>(R.id.textView8).text = try{
                    SimpleDateFormat("dd/MM/yyyy HH:mm:ss").format(Date(item.getJSONObject("lastAlarmNotificationTmsMs").getLong("N")))
                }catch (error: JSONException) {
                    Log.e("HomeAlarmApp", "JSON Exception", error);
                    "ERROR";
                }

                // set Sensors
                try{
                    val sensorsArray:JSONArray = item.getJSONObject("sensors").getJSONArray("L");
                    for (i in 0 until sensorsArray.length()){

                        // {"M":{"alarm":{"BOOL":false},"lastCheck":{"N":"0"},"deviceCode":{"S":"PROX_1"},"value":{"S":"0.0000"},"enabled":{"BOOL":false}}},
                        val alarm:Boolean = sensorsArray.getJSONObject(i).getJSONObject("M").getJSONObject("alarm").getBoolean("BOOL");
                        val lastCheck:Long = sensorsArray.getJSONObject(i).getJSONObject("M").getJSONObject("lastCheck").getLong("N");
                        val deviceCode:String = sensorsArray.getJSONObject(i).getJSONObject("M").getJSONObject("deviceCode").getString("S");
                        val value:String = sensorsArray.getJSONObject(i).getJSONObject("M").getJSONObject("value").getString("S");
                        val enabled:Boolean = sensorsArray.getJSONObject(i).getJSONObject("M").getJSONObject("enabled").getBoolean("BOOL");

                        var tmpChkBox:CheckBox? = null;
                        var tmpStatus:TextView? = null;
                        when(deviceCode){
                            "PROX_1" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox1);
                                tmpStatus = findViewById<TextView>(R.id.prox1StatusText);
                            }
                            "PROX_2" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox2);
                                tmpStatus = findViewById<TextView>(R.id.prox2StatusText);
                            }
                            "PROX_3" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox3);
                                tmpStatus = findViewById<TextView>(R.id.prox3StatusText);
                            }
                            "PROX_4" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox4);
                                tmpStatus = findViewById<TextView>(R.id.prox4StatusText);
                            }
                            "PROX_5" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox5);
                                tmpStatus = findViewById<TextView>(R.id.prox5StatusText);
                            }
                            "PIR_1" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox6);
                                tmpStatus = findViewById<TextView>(R.id.pir1StatusText);
                            }
                            "PIR_2" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox7);
                                tmpStatus = findViewById<TextView>(R.id.pir2StatusText);
                            }
                            "PIR_3" -> {
                                tmpChkBox = findViewById<CheckBox>(R.id.checkBox8);
                                tmpStatus = findViewById<TextView>(R.id.pir3StatusText);
                            }
                        }

                        if(tmpChkBox != null && tmpStatus != null) {
                            tmpChkBox.isEnabled = true;
                            tmpChkBox.isChecked = enabled;

                            if(alarm) {
                                tmpStatus.text = "ALARM";
                                tmpStatus.typeface = Typeface.defaultFromStyle(Typeface.BOLD);
                                tmpStatus.setTextColor(Color.RED);
                            }
                            else{
                                tmpStatus.text = "NORMAL";
                                tmpStatus.typeface = Typeface.defaultFromStyle(Typeface.NORMAL);
                                tmpStatus.setTextColor(Color.LTGRAY);
                            }
                        }


                    }

                } catch (error: JSONException) {
                    Log.e("HomeAlarmApp", "JSON Exception", error);
                    null;
                }


                /*
            val builder = AlertDialog.Builder(this)
            builder.setTitle("OK")
            builder.setNeutralButton("Ok") { dialog, which ->
                Toast.makeText(applicationContext,
                    "Ok", Toast.LENGTH_SHORT).show()
            }
            builder.show()
             */

            });
        }

    }

    private fun getStatusErrorCallback(it: ApiException){

        Log.e("MyAmplifyApp", "POST failed", it);

        this@MainActivity.runOnUiThread(java.lang.Runnable {

            val now:String = DateTimeFormatter
                .ofPattern("dd/MM/yyyy HH:mm:ss")
                .withZone(ZoneOffset.UTC)
                .format(Instant.now());
            val refreshButton:Button = findViewById<Button>(R.id.button3);
            val progressBar1:ProgressBar = findViewById<ProgressBar>(R.id.progressBar);
            val lastUpd:TextView = findViewById<TextView>(R.id.textView2);

            //refreshButton.isEnabled = false
            refreshButton.visibility = View.VISIBLE;
            progressBar1.visibility = View.GONE;
            lastUpd.text = now;

            val builder = AlertDialog.Builder(this)
            builder.setTitle("Error")
            builder.setMessage(it.localizedMessage)

            builder.setNeutralButton("OK") { dialog, which ->
                Toast.makeText(applicationContext,
                    "ok", Toast.LENGTH_SHORT).show()
            }
            builder.show()

        })

    }
}