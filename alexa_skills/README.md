# Clues finder

Clues finder is an Alexa skill which runs on AWS lambda and it is responsible for giving out hints to the users during gameplay.

The skill has 2 components -
* Frontend - Managed by Alexa Developer Console
* Backend - Managed by Amazon AWS

## Alexa Developer Console
Here the skill is created. The dashboard has options for different kinds of skills. In our case, we need a custom skill.
A comprehensive guide to create an Alexa skill is given [here](https://developer.amazon.com/en-US/docs/alexa/devconsole/create-a-skill-and-choose-the-interaction-model.html).

### Skill wake-up word
Alexa can start listening with the words "Alexa", "Amazon" and "Computer". Similarly, we need a wake word for the skill as well apart from Alexa's wake word.  This can be configured in the dashboard. However, it should be atleast 2 words long.
Wake-up word for Clues finder is "Stasis world".
#### Alexa wake word dashboard
![Alexa wake word dashboard](https://github.com/ubilab-escape/environment/raw/master/images/alexa_dashboard.png)

### Commands to which it can listen and respond
For every distinguishable action, we need to specify a set of sentences which will envoke a certain callback. These callbacks are programmed with python which handles the response of a command.

#### Frontend showing the sentences which will trigger the safe hint callback function.
![safe command callback](https://github.com/ubilab-escape/environment/raw/master/images/safe_command.png)

## Alexa skill backend
The backend of the skill lives on AWS lambda. Callbacks of all the commands are written in python and uploaded to this repository.
A comprehensive guide to create the backend for the skill is given [here](https://developer.amazon.com/en-US/docs/alexa/custom-skills/host-a-custom-skill-as-an-aws-lambda-function.html).

## Downloading the skill to your Alexa device

After developing the skill, we need to load the skill to the device. We can do that by enabling the skill in the Alexa app *Skills and Games* section under the *dev* tab.

![Alexa skill in the app](https://github.com/ubilab-escape/environment/raw/master/images/skills_device.jpg)
