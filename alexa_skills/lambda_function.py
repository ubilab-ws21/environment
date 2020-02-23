# -*- coding: utf-8 -*-

# This module handles all the alexa callbacks.
import boto3
import decimal
import json
import logging
import ask_sdk_core.utils as ask_utils
import time

from ask_sdk_core.skill_builder import CustomSkillBuilder
from ask_sdk_core.dispatch_components import AbstractRequestHandler
from ask_sdk_core.dispatch_components import AbstractExceptionHandler
from ask_sdk_core.handler_input import HandlerInput
from ask_sdk_model import Response
from botocore.exceptions import ClientError

import config

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# Create a dynamo db client.
dynamodb = boto3.client('dynamodb')
CLUE_COUNTER = 0
CURRENT_PUZZLE = None


# Helper class to convert a DynamoDB item to JSON.
class DecimalEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, decimal.Decimal):
            if abs(o) % 1 > 0:
                return float(o)
            else:
                return int(o)
        return super(DecimalEncoder, self).default(o)

class LaunchRequestHandler(AbstractRequestHandler):
    """Handler for Skill Launch."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_request_type("LaunchRequest")(handler_input)

    def handle(self, handler_input):
        speak_output = "What can stasis do for you?"
        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )


class HelloWorldIntentHandler(AbstractRequestHandler):
    """Handler for Hello World Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("HelloWorldIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "I did not understand that. Please repeat."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )

class NoHandler(AbstractRequestHandler):
    """Handler for No Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("NoIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "Ok."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )


class HelpIntentHandler(AbstractRequestHandler):
    """Handler for Help Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("AMAZON.HelpIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "I can give you some hints."
        ask_output = "What do you want?"
        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(ask_output)
                .response
        )


class CancelOrStopIntentHandler(AbstractRequestHandler):
    """Single handler for Cancel and Stop Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return (ask_utils.is_intent_name("AMAZON.CancelIntent")(handler_input) or
                ask_utils.is_intent_name("AMAZON.StopIntent")(handler_input))

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "Goodbye!"

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )


class SessionEndedRequestHandler(AbstractRequestHandler):
    """Handler for Session End."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_request_type("SessionEndedRequest")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response

        # Any cleanup logic goes here.

        return handler_input.response_builder.response


class GiveClueHandler(AbstractRequestHandler):
    """
    This intent gives the hint depending on the current game level.
    It is not complete and not ready to use. Although, it can be extended
    easily.
    The idea is, an external script will update the next hint to give depending
    on the game scenario in the dynamodb. This handler can pick up that from
    the db and read out relevant hint.
    """
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("GiveClue")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        logger.info("I'm in clue handler.")
        try:
            response = dynamodb.get_item(TableName='stassis_info',
                    Key={
                        'game_id': {'N': '1'}
                        }
                    )
        except ClientError as e:
            logger.error(e.response['Error']['Message'])
        else:
            item = response['Item']
            logger.info("GetItem succeeded:")
            logger.info(json.dumps(item, indent=4, cls=DecimalEncoder))
            active_puzzles = item['levels']['S'].split(",")
        if len(active_puzzles) == 1:
            try:
                old_puzzle = CURRENT_PUZZLE
                CURRENT_PUZZLE = active_puzzles[0]
                if old_puzzle != CURRENT_PUZZLE:
                    CLUE_COUNTER = 0
                clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
                CLUE_COUNTER += 1
                clue_sentence += ". Do you need another clue?"
                return (
                        handler_input.response_builder
                        .speak(clue_sentence)
                        .ask("Do you need another clue?")
                        .response
                        )
            except IndexError as e:
                clue_sentence = "I dont have any other clues for this puzzle."
                return (
                        handler_input.response_builder
                        .speak(clue_sentence)
                        .response
                        )
        else:
            askable_questions = [config.ACTUAL_NAME_TO_PUZZLE[puzzle.strip()]
                    for puzzle in active_puzzles]
            clue_sentence = "I can give you hint for" + " and ".join(askable_questions)
            clue_sentence += ". For which one you need the hint?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("For which one you need the hint?")
                    .response
                    )


class GlobeHandler(AbstractRequestHandler):
    """This intent gives hint for the globe puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("Globe")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "globe"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class SafeHandler(AbstractRequestHandler):
    """This intent gives hint for the safe puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("Safe")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "safe"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class Door2Handler(AbstractRequestHandler):
    """This intent gives hint for the door2 puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("DoorTwo")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "door2"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class RetrievePrototypeHandler(AbstractRequestHandler):
    """This intent gives hint for the prototype puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("RetrievePrototype")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "retrieving_prototype"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class PrototypeHandler(AbstractRequestHandler):
    """This intent gives hint for the prototype puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("Prototype")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "prototype"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class OverrideServerHandler(AbstractRequestHandler):
    """This intent gives hint for the server puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("OverrideServer")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        try:
            old_puzzle = CURRENT_PUZZLE
            CURRENT_PUZZLE = "server"
            if old_puzzle != CURRENT_PUZZLE:
                CLUE_COUNTER = 0
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class YesHandler(AbstractRequestHandler):
    """This intent gives hint for the server puzzle."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("Yes")(handler_input)

    def handle(self, handler_input):
        global CLUE_COUNTER
        global CURRENT_PUZZLE
        if not CURRENT_PUZZLE:
            return (
                    handler_input.response_builder
                    .speak("What can stasis do for you?")
                    .response
                    )
        try:
            clue_sentence = config.CLUES_TABLE[CURRENT_PUZZLE][CLUE_COUNTER]
            CLUE_COUNTER += 1
            clue_sentence += ". Do you need another clue?"
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .ask("Do you need another clue?")
                    .response
                    )
        except IndexError as e:
            clue_sentence = "I dont have any other clues for this puzzle."
            return (
                    handler_input.response_builder
                    .speak(clue_sentence)
                    .response
                    )

class IntentReflectorHandler(AbstractRequestHandler):
    """The intent reflector is used for interaction model testing and debugging.
    It will simply repeat the intent the user said. You can create custom handlers
    for your intents by defining them above, then also adding them to the request
    handler chain below.
    """
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_request_type("IntentRequest")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        intent_name = ask_utils.get_intent_name(handler_input)
        speak_output = "You just triggered " + intent_name + "."

        return (
            handler_input.response_builder
                .speak(speak_output)
                # .ask("add a reprompt if you want to keep the session open for the user to respond")
                .response
        )

class CatchAllExceptionHandler(AbstractExceptionHandler):
    """Generic error handling to capture any syntax or routing errors. If you receive an error
    stating the request handler chain is not found, you have not implemented a handler for
    the intent being invoked or included it in the skill builder below.
    """
    def can_handle(self, handler_input, exception):
        # type: (HandlerInput, Exception) -> bool
        return True

    def handle(self, handler_input, exception):
        # type: (HandlerInput, Exception) -> Response
        logger.error(exception, exc_info=True)

        speak_output = "Sorry, I had trouble doing what you asked. Please try again."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )

# The SkillBuilder object acts as the entry point for your skill, routing all request and response
# payloads to the handlers above. Make sure any new handlers or interceptors you've
# defined are included below. The order matters - they're processed top to bottom.

sb = CustomSkillBuilder()

sb.add_request_handler(LaunchRequestHandler())
sb.add_request_handler(HelloWorldIntentHandler())
sb.add_request_handler(HelpIntentHandler())
sb.add_request_handler(CancelOrStopIntentHandler())
sb.add_request_handler(SessionEndedRequestHandler())
sb.add_request_handler(GiveClueHandler())
sb.add_request_handler(GlobeHandler())
sb.add_request_handler(SafeHandler())
sb.add_request_handler(Door2Handler())
sb.add_request_handler(RetrievePrototypeHandler())
sb.add_request_handler(PrototypeHandler())
sb.add_request_handler(OverrideServerHandler())
sb.add_request_handler(YesHandler())
sb.add_request_handler(NoHandler())
sb.add_request_handler(IntentReflectorHandler()) # make sure IntentReflectorHandler is last so it doesn't override your custom intent handlers

sb.add_exception_handler(CatchAllExceptionHandler())

lambda_handler = sb.lambda_handler()
