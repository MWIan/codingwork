'''
NORMAL BLACKJACK RULES APPLY TO DEALER -> HIT TO 16
FOR PLAYERS:
- PLAYER CAN ONLY SPLIT UP TO 4 HANDS (3 SPLITS MAX) - CAN DOUBLE DOWN
- ON ACES SPLIT, ONE CARD ONLY NO DOUBLE DOWN

SIMULATION
- IF A PLAYER'S HAND HAS CARDS VALUING 11 OR LESS - WILL ALWAYS HIT (OR DOUBLE DOWN)
'''


import copy
import random
import json

def createDecks(cardList, numDecks):
    deck = []
    for i in range(0, numDecks):
        deck.extend(copy.deepcopy(cardList))
    return deck

def randomizeDeck(deck):
    lowBound = random.randint(15, 30)
    highBound = random.randint(40, 55)
    timesToShuffle = random.randint(lowBound, highBound)
    for x in range(0, timesToShuffle):
        random.shuffle(deck)
    return deck

def getCardValue(card, deck):
    return deck[card].value

def getHandValue(handObj, deck):
    val = 0
    for card in handObj.cards:
        val += deck[card].value
    
    # if bust, go back and use alt value (if lower than reg value (for aces))
    if val > 21:
        for card in handObj.cards:
            if deck[card].altValue is not None and deck[card].altValue < deck[card].value:
                val -= deck[card].value
                val += deck[card].altValue
                if val <= 21: break
    return val

class Hand:
    def __init__(self, card):
        self.cards = [card] # card is index in deck
        self.doubled = False # hand hasn't been doubled

    def addCard(self, card):
        self.cards.append(card)

    def splitHand(self):
        # removes second card from original hand and adds it to a new hand, then returns the new hand
        card = self.cards[1]
        self.cards.pop(1)
        return Hand(card)
    
    def recordHand(self, deck):
        for card in self.cards: print(deck[card].card, end=", ")
        if self.doubled: print("Doubled")
        else: print()
    
    def toDict(self, deck):
        cardList = []
        for card in self.cards: cardList.append(deck[card].card)
        return {
            "cards":cardList,
            "doubled":self.doubled
        }

class Player:
    def __init__(self, initialHand):
        self.hands = []
        self.hands.append(initialHand)
        self.numSplits = 0

    def playHand(self, nextCard, deck):
        # play hands
        i = 0
        while i < len(self.hands):
            # look to see if hand has been split (will only have one card), then deal it a card
            if len(self.hands[i].cards) < 2:
                # draw a card
                self.hands[i].addCard(nextCard)
                nextCard += 1
                # if ace, then can't draw any more cards
                if getCardValue(self.hands[i].cards[0], deck) == 11:
                    i += 1
                    continue

            # check hand for same card, if they match then randomly decide whether to split or not
            # can only split if player has 3 hands or less
            if getCardValue(self.hands[i].cards[0], deck) == getCardValue(self.hands[i].cards[1], deck) and len(self.hands) < 3:
                # choose whether to split or not
                toSplit = random.randint(0,1) # 0 is split, 1 is no split
                if toSplit == 0:
                    self.hands.append(self.hands[i].splitHand())
                    self.numSplits += 1
                    continue # start over at top of while stmt with this hand
            
            # after any possible splits, we will have 2 cards here
            toDouble = random.randint(0,1) # random choice on whether to double
            if toDouble == 0: # if doubled, one card only then move to next hand
                self.hands[i].addCard(nextCard)
                self.hands[i].doubled = True
                nextCard += 1
                i += 1
                continue

            # while the cards add to 11 or less, then we will hit 100%
            while getHandValue(self.hands[i], deck) <= 11:
                self.hands[i].addCard(nextCard)
                nextCard += 1

            # now that we have a hand totaling over 11, make a decision on whether to hit or not
            # do 40% chance to hit, 60% chance to stay
            # if cards add to 21, then never hit
            toHit = random.randint(0,10)
            while toHit <= 4 and getHandValue(self.hands[i], deck) < 21:
                self.hands[i].addCard(nextCard)
                nextCard += 1
                toHit = random.randint(0,10)

            i += 1

        return nextCard # returns next available card in deck
    
    def recordHand(self, deck):
        print("Player hand(s):")
        for hand in self.hands:
            hand.recordHand(deck)

    def handsToDict(self, deck):
        handsList = []
        i = 1
        for hand in self.hands:
            handsList.append({"handNum":i, "hand":hand.toDict(deck)})
            i += 1
        return handsList


class DealerHand:
    def __init__(self, card):
        self.cards = [card] # card is index in deck
    def addCard(self, card):
        self.cards.append(card)
    
    def playHand(self, nextCard, deck):
        while getHandValue(self, deck) < 17:
            self.addCard(nextCard)
            nextCard += 1

        return nextCard

    def recordHand(self, deck):
        print("Dealer hand:")
        for card in self.cards: print(deck[card].card, end=", ")
        print()
    
    def toDict(self, deck):
        cardList = []
        for card in self.cards: cardList.append(deck[card].card)
        handValue = getHandValue(self, deck)
        return {"cards":cardList, "value":handValue}

class Card:
    def __init__(self, card, suit, value, altValue):
        self.card = card
        self.suit = suit
        self.value = value
        self.altValue = altValue # altvalue should be lower than value

def createCards():
    suits = ['Spades', 'Clubs', 'Hearts', 'Diamonds']
    cardList = []
    for suit in suits:
        cardList.append(Card('A', suit, 11, 1))
        cardList.append(Card('2', suit, 2, None))
        cardList.append(Card('3', suit, 3, None))
        cardList.append(Card('4', suit, 4, None))
        cardList.append(Card('5', suit, 5, None))
        cardList.append(Card('6', suit, 6, None))
        cardList.append(Card('7', suit, 7, None))
        cardList.append(Card('8', suit, 8, None))
        cardList.append(Card('9', suit, 9, None))
        cardList.append(Card('10', suit, 10, None))
        cardList.append(Card('J', suit, 10, None))
        cardList.append(Card('Q', suit, 10, None))
        cardList.append(Card('K', suit, 10, None))
    return cardList

#numDecks = 6
#deck = randomizeDeck(createDecks(createCards(), numDecks))


class Game:
    def __init__(self, numPlayers, numDecks, numPlays, continuousShuffling):
        # create game deck
        self.deck = randomizeDeck(createDecks(createCards(), numDecks))

        allHands = []
        self.nextCard = 0 # index of card to give out

        for j in range(0, numPlays):
            # continuous shuffling adds old cards to the deck -> gotta work out logistics
            # deal player 1st card
            self.players = []
            for i in range(0, numPlayers):
                self.players.append(Player(Hand(self.nextCard)))
                self.nextCard += 1
            # deal dealer 1st card
            self.dealer = DealerHand(self.nextCard)
            self.nextCard += 1

            # deal player 2nd cards
            for i in range(0, numPlayers):
                self.players[i].hands[0].addCard(self.nextCard) # add card to players first hand
                self.nextCard += 1
            
            # deal dealer 2nd card
            self.dealer.addCard(self.nextCard)
            self.nextCard += 1

            self.playHands()

            #self.dealer.recordHand(self.deck)
            #for player in self.players: player.recordHand(self.deck)
            allHands.append({"gameNum":j+1, "game":self.createGameDict()})
        
        self.gameInfo = {
            "numDecks":numDecks,
            "numGames":numPlays,
            "numPlayers":numPlayers,
            "games":allHands
        }

    def playHands(self):
        # check dealer's hand for blackjack
        if getHandValue(self.dealer, self.deck) == 21:
            return
        # go through each player and play their hand
        for player in self.players:
            self.nextCard = player.playHand(self.nextCard, self.deck)
        
        # play dealer's hand
        self.nextCard = self.dealer.playHand(self.nextCard, self.deck)
    
    def createGameDict(self):
        dealerObj = self.dealer.toDict(self.deck)
        playersList = []
        i = 1
        for player in self.players:
            playersList.append({"playerNum":i, "hands":player.handsToDict(self.deck), "numSplits":player.numSplits})
            i += 1

        return {"dealer":dealerObj, "players":playersList}

def playGame(numPlayers, numDecks, numGames, continuousShuffle):
    game = Game(numPlayers, numDecks, numGames, continuousShuffle) # num of players (hands to deal), num decks, num times to play, if should continually shuffle used cards
    return game.gameInfo

if __name__ == "__main__":
    dataset = []
    numSimulations = 15000
    numPlayers = 6
    numDecks = 6
    numGames = 4
    continuousShuffle = False
    for i in range(0, numSimulations):
        dataset.append(playGame(numPlayers, numDecks, numGames, continuousShuffle))

    data = {
        "numSimulations":numSimulations,
        "data":dataset
    }
    json_object = json.dumps(data, indent = 2) 
    f = open("datafile.txt", "a")
    f.write(json_object)
    f.close()
    # store data