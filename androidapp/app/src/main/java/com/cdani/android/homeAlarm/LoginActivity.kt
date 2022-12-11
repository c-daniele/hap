package com.cdani.android.homeAlarm

import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.*
import com.amplifyframework.AmplifyException
import com.amplifyframework.api.aws.AWSApiPlugin
import com.amplifyframework.api.rest.RestOperation
import com.amplifyframework.api.rest.RestOptions
import com.amplifyframework.auth.AuthException
import com.amplifyframework.auth.AuthSession
import com.amplifyframework.auth.cognito.AWSCognitoAuthPlugin
import com.amplifyframework.auth.cognito.AWSCognitoAuthSession
import com.amplifyframework.auth.result.AuthSessionResult
import com.amplifyframework.core.Amplify

class LoginActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Add these lines to add the `AWSApiPlugin` and `AWSCognitoAuthPlugin`
        Amplify.addPlugin(AWSApiPlugin())
        Amplify.addPlugin(AWSCognitoAuthPlugin())
        Amplify.configure(applicationContext)

        setContentView(R.layout.activity_login)
    }

    fun onLoginClick(view: View){

        Log.d("LoginActivity::onLoginClick", "START");

        val loginButton: Button = findViewById<Button>(R.id.button4)
        val progressBar: ProgressBar = findViewById<ProgressBar>(R.id.progressBar2)
        val usernameInput: EditText = findViewById<EditText>(R.id.usernameInputText)
        val passwordInput: EditText = findViewById<EditText>(R.id.passwordInputText)
        val errorMessage: TextView = findViewById<TextView>(R.id.textView11)
        //loginButton.isEnabled = false

        // temporary disable UX components
        loginButton.isEnabled = false;
        progressBar.visibility = View.VISIBLE;
        errorMessage.visibility = View.GONE;
        errorMessage.text = "";

        try {

            //var loginOk:Boolean = false;
            Log.i("HomeAlarmApp", "Initialized Amplify")

            Amplify.Auth.signIn(usernameInput.text.toString(), passwordInput.text.toString(),
                { result ->
                    if (result.isSignInComplete) {
                        Log.i("HomeAlarmApp", "Sign in succeeded")

                        this@LoginActivity.runOnUiThread(java.lang.Runnable {
                            progressBar.visibility = View.GONE;
                            loginButton.isEnabled = true;
                            val mainIntent:Intent = Intent(this, MainActivity::class.java);
                            startActivity(mainIntent)
                        })

                    } else {
                        Log.i("HomeAlarmApp", "Sign in not complete")
                        this@LoginActivity.runOnUiThread(java.lang.Runnable {
                            errorMessage.visibility = View.VISIBLE;
                            loginButton.isEnabled = true;
                            errorMessage.typeface = Typeface.defaultFromStyle(Typeface.BOLD);
                            errorMessage.setTextColor(Color.RED);
                            errorMessage.text = "Error"
                        })
                    }
                },
                {
                    Log.e("HomeAlarmApp", "Failed to sign in", it)
                    this@LoginActivity.runOnUiThread(java.lang.Runnable {
                        errorMessage.visibility = View.VISIBLE;
                        loginButton.isEnabled = true;
                        errorMessage.typeface = Typeface.defaultFromStyle(Typeface.BOLD);
                        errorMessage.setTextColor(Color.RED);
                        errorMessage.text = if(it.cause != null) it.cause!!.localizedMessage else it.localizedMessage;
                    })
                }
            )

        } catch (error: AmplifyException) {
            Log.e("HomeAlarmApp", "Could not initialize Amplify", error)
            this@LoginActivity.runOnUiThread(java.lang.Runnable {
                errorMessage.visibility = View.VISIBLE;
                loginButton.isEnabled = true;
                errorMessage.text = if(error.cause != null) error.cause!!.localizedMessage else error.localizedMessage;
            })
        }

        Log.d("LoginActivity::onLoginClick", "END");

    }
}