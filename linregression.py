from ib_insync import *
import fibs
import datetime
import time
import numpy as np
import math
import tickers

class Order:
    def __init__(self, tickerObj, numShares, price, target, stop, order, profitOrder, stopOrder):
        self.tickerObj = tickerObj
        self.numShares = numShares
        self.price = price
        self.target = target
        self.stop = stop
        self.lastCandle = getLastCandle(tickerObj["marketData"])
        self.order = order
        self.profitOrder = profitOrder
        self.stopOrder = stopOrder
    
    # def logCancel(self):
    #     filename = datetime.datetime.now().strftime("%Y-%m-%d") + ".txt"
    #     with open("./orders/"+filename, 'a') as f:
    #         f.write(self.tickerObj["ticker"].symbol + ", " + self.order.action)
    def logStopLoss(self):
        filename = datetime.datetime.now().strftime("%Y-%m-%d") + ".txt"
        with open("./orders/"+filename, 'a') as f:
            # format: symbol, ordertype, numshares, price, target, stop, last candlehigh, last candlelow, time order was placed
            f.write(self.tickerObj["ticker"].symbol + ", " + self.order.order.action + ", " + str(self.numShares) + ", " + str(self.price) + ", " + str(self.target) + ", " + str(self.stop) + ", " + str(self.lastCandle[1]) + ", " + str(self.lastCandle[2]) + ", " + self.tickerObj["orderTime"].strftime("%H:%M:%S") + ", STOPPED\n")
    
    def logProfit(self):
        filename = datetime.datetime.now().strftime("%Y-%m-%d") + ".txt"
        with open("./orders/"+filename, 'a') as f:
            # format: symbol, ordertype, numshares, price, target, stop, last candlehigh, last candlelow, time order was placed
            f.write(self.tickerObj["ticker"].symbol + ", " + self.order.order.action + ", " + str(self.numShares) + ", " + str(self.price) + ", " + str(self.target) + ", " + str(self.stop) + ", " + str(self.lastCandle[1]) + ", " + str(self.lastCandle[2]) + ", " + self.tickerObj["orderTime"].strftime("%H:%M:%S") + ", TARGET\n")


def ceil_dt(dt, delta):
    return dt + (dt.min - dt) % delta

def get_last_date():
    # get day before today
    yesterday = datetime.date.today() - datetime.timedelta(days = 1)
    return yesterday

def placeBracketOrder(tickerObj, orderType, contract, quantity, price, takeProfit, stopLoss):
    #orderType should be 'BUY' or 'SELL'
    newOrder = app.bracketOrder(orderType, quantity, price, takeProfit, stopLoss)
    i = 0
    for o in newOrder: # mainorder, then takeprofit, then stoploss
        order = app.placeOrder(contract, o)
        if i == 0:
            mainOrder = order
        elif i == 1:
            profitOrder = order
        elif i == 2:
            stopOrder = order
        order
        i += 1
    orderObj = Order(tickerObj, quantity, price, takeProfit, stopLoss, mainOrder, profitOrder, stopOrder)
    return orderObj, mainOrder, profitOrder, stopOrder

def createTickerObjects(tickers):
    # input list of tickers
    # output list of ticker objects
    output = []
    for ticker in tickers:
        obj = {
            "ticker":ticker,
            "marketData":None,
            "orderIP":False,
            "orderTime":None,
            "nextTradeTime":None
        }
        output.append(obj)
    return output

def selectTickerObjects(tickerObjs):
    # input list of ticker objects
    # returns two lists:
        # first list: first 50 objects with opposing 20/40 regression lines
        # second list: objects not in first list
    included = []
    notIncluded = []
    count = 0
    for ticker in tickerObjs:
        #print(ticker["ticker"].symbol)
        if count < 50:
            ticker["marketData"] = app.reqHistoricalData(ticker["ticker"], '', '3 D', '5 mins', 'MIDPOINT', True, 1, False, [], 0)
            a, b, c, slope20 = getRegression(ticker["marketData"], 20)
            a, b, c, slope40 = getRegression(ticker["marketData"], 40)
            if (slope40 < 0 and slope20 > 0) or (slope40 > 0 and slope20 < 0):
                included.append(ticker)
                ticker["marketData"] = app.reqHistoricalData(ticker["ticker"], '', '3 D', '5 mins', 'MIDPOINT', True, 1, True, [], 0)
                count += 1
                print(ticker["ticker"].symbol)
            else:
                notIncluded.append(ticker)
                #app.cancelHistoricalData(ticker["marketData"])
        else:
            notIncluded.append(ticker)
    return included, notIncluded


def getRegression(marketData, count):
    # input 1 market data obj and count num of bars
    # returns last price, standard deviation, last point in regression line, slope
    bars = marketData[len(marketData)-count:]
    closes = [item.close for item in bars]
    #print(closes)
    x = list(range(1,count+1))

    # y = mx + b
    m, b = np.polyfit(x, closes, 1)
    
    std = calculateRSD(closes, count, m, b) # residual standard deviation
    #print("residual std: " + str(std))

    mid = m*count+b

    return closes[len(closes)-1], std, mid, m


def calculateRSD(data, count, m, b):
    # data -> array of nums
    # count -> num items
    # m -> slope
    # b -> x-intercept
    sum = 0
    for x in range(1,count+1):
        residual = abs(data[x-1] - (m*x+b))
        sum += residual*residual
    sum /= count-2

    sres = math.sqrt(sum)
    return sres

def numSharesFromBudget(toSpend, stockPrice):
    return int(toSpend/stockPrice)

def numSharesForProfit(desiredProfit, stockPrice, takeProfit):
    profitPerShare = abs(takeProfit - stockPrice)
    numShares = int(desiredProfit / profitPerShare)
    return numShares

def replaceTicker(tickerObj):
    # input ticker object to be replaced in list of 50
    # find a new ticker to replace
    app.cancelHistoricalData(tickerObj["marketData"])
    tickerFound = False

    for ticker in tickersNotInUse:
        ticker["marketData"] = app.reqHistoricalData(ticker["ticker"], '', '3 D', '5 mins', 'MIDPOINT', True, 1, False, [], 0)
        a, b, c, slope20 = getRegression(ticker["marketData"], 20)
        a, b, c, slope40 = getRegression(ticker["marketData"], 40)
        if (slope40 < 0 and slope20 > 0) or (slope40 > 0 and slope20 < 0):
            if (ticker["nextTradeTime"] == None or datetime.datetime.now > ticker["nextTradeTime"]):
                ticker["marketData"] = app.reqHistoricalData(ticker["ticker"], '', '3 D', '5 mins', 'MIDPOINT', True, 1, True, [], 0)
                # swap ticker objects
                tickersToUse.remove(tickerObj)
                tickersNotInUse.append(tickerObj)
                tickersToUse.append(ticker)
                tickersNotInUse.remove(ticker)
                tickerFound = True
                break
        #app.cancelHistoricalData(ticker["marketData"])
    if not tickerFound:
        # if ticker hasnt been found, may as well still replace it with first tradeable one
        for ticker in tickersNotInUse:
            if (ticker["nextTradeTime"] == None or datetime.datetime.now > ticker["nextTradeTime"]):
                # replace with first tradeable ticker
                ticker["marketData"] = app.reqHistoricalData(ticker["ticker"], '', '3 D', '5 mins', 'MIDPOINT', True, 1, True, [], 0)
                tickersToUse.remove(tickerObj)
                tickersNotInUse.append(tickerObj)
                tickersToUse.append(ticker)
                tickersNotInUse.remove(ticker)
    print("Replacing ticker")

def getLastCandle(marketData):
    # input bardatalist from market data
    # returns info of last bar
    lastBar = marketData[len(marketData)-1]
    return (lastBar.open, lastBar.high, lastBar.low, lastBar.close)

def manageOrders(now):
    # input now: datetime
    for order in ordersIP:
        if order.order.orderStatus.status != 'Filled' and now - order.tickerObj["orderTime"] > datetime.timedelta(minutes=1):
            # cancel order after 60 seconds with no fill
            app.cancelOrder(order.order.order)
            order.tickerObj["orderIP"] = False

            # update ordersIP and log the order
            ordersIP.remove(order)
        elif order.profitOrder.orderStatus.status == 'Filled':
            order.tickerObj["orderIP"] = False
            order.tickerObj["nextTradeTime"] = now + datetime.timedelta(minutes=45) # can trade again 45 mins after position gets closed
            
            # update ordersIP and log the order
            ordersIP.remove(order)
            order.logProfit()
        elif order.stopOrder.orderStatus.status == 'Filled':
            order.tickerObj["orderIP"] = False
            order.tickerObj["nextTradeTime"] = now + datetime.timedelta(minutes=45) # can trade again 45 mins after position gets closed
            
            # update ordersIP and log the order
            ordersIP.remove(order)
            order.logStopLoss()


		### Main ###
app = IB()
app.connect("127.0.0.1", 7497, 143)


tickerObjs = createTickerObjects(tickers.tickers)
#tickerObjs = createTickerObjects([Stock("ABBV", "SMART", "USD")])

ordersIP = [] # list of Orders
tickersToUse, tickersNotInUse = selectTickerObjects(tickerObjs)
print("num tickers to use:")
print(len(tickersToUse))
print("num tickers not in use:")
print(len(tickersNotInUse))

now = datetime.datetime.now()
replaceTickersTime = now + datetime.timedelta(minutes=5)

#debugging
# abbv = tickersToUse[0]
# buyPrice = 113
# stop = 112
# takeprof = 118
# thisOrder, abbv["order"], abbv["profitOrder"], abbv["stopOrder"] = placeBracketOrder(abbv, 'BUY', abbv["ticker"], 20, buyPrice, takeprof, stop)
# abbv["orderIP"] = True
# abbv["orderTime"] = now
# ordersIP.append(thisOrder)
# thisOrder.logProfit()
#end debugging


print("Starting algo:")

while True:
    app.sleep(5)
    now = datetime.datetime.now()
    manageOrders(now)

    for item in tickersToUse:
        last, std20, mid20, slope20 = getRegression(item["marketData"], 20) # last 20 candles
        last, std40, mid40, slope40 = getRegression(item["marketData"], 40) # last 40 candles
        l220 = mid20 - std20*2
        l240 = mid40 - std40*2
        l320 = mid20 - std20*3
        u220 = mid20 + std20*2
        u240 = mid40 + std40*2
        u320 = mid20 + std20*3

        # theory
        # get last 40 15min candles. check linear regression slope. If slopes up/down by a fair bit (what # cutoff?)
            # then check regression of last 20 15min candles. If that slopes in opposite direction AND the 2nd deviation lines
            # along with price all intersect then could be a good chance of reversal or at least a pullback
                    # possibly stoploss at 3rd deviation line and takeprofit at the middle trendline (of 20-length channel)
        
        # regression line intersection should be within 0.2%(?) of price
            # put buy order for whatever line would make most profit / least stoploss

            # 20/40 slopes must be between 0.01 and 0.2 and in opposite direction
            # go long when 40 slope is positive and 20 slope is negative
            # go short when 40 slope is negative and 20 slope is positive
        if slope40 > 0.01 and slope40 < 0.2 and slope20 < -0.01 and slope20 > -0.2:
            # check lower standard deviations and price to see if they are all close
            if last <= l220 and (abs(l220 - l240) / last) < 0.002:
                # go long if can
                if (not item["orderIP"] and (item["nextTradeTime"] == None or now > item["nextTradeTime"])):
                    # go long at last price with takeprofit of mid20 and stoploss of l320
                    # buy at last price with stoploss at 1 std below, and takeprofit 1.5 std above
                    buyPrice = round(last, 2)
                    stop = round(buyPrice - std20*2, 2)
                    takeprof = round(buyPrice + std20*2, 2)
                    thisOrder, item["order"], item["profitOrder"], item["stopOrder"] = placeBracketOrder(item, 'BUY', item["ticker"], numSharesForProfit(25, buyPrice, takeprof), buyPrice, takeprof, stop)
                    item["orderIP"] = True
                    item["orderTime"] = now
                    ordersIP.append(thisOrder)
                    replaceTicker(item) # replace ticker when trade occurs

        elif slope40 < -0.01 and slope40 > -0.2 and slope20 > 0.01 and slope20 < 0.2:
            # check upper standard deviations and price to see if they are all close
            if last >= u220 and (abs(u220 - u240) / last) < 0.002:
            # go short if can
                if (not item["orderIP"] and (item["nextTradeTime"] == None or now > item["nextTradeTime"])):
                    # go short at last price with takeprofit of mid20 and stoploss of u320
                    # sell at last price with stoploss at 1 std above, and takeprofit 1.5 std below
                    sellPrice = round(last, 2)
                    stop = round(sellPrice + std20*2, 2)
                    takeprof = round(sellPrice - std20*2, 2)
                    thisOrder, item["order"], item["profitOrder"], item["stopOrder"] = placeBracketOrder(item, 'SELL', item["ticker"], numSharesForProfit(25, sellPrice, takeprof), sellPrice, takeprof, stop)
                    item["orderIP"] = True
                    item["orderTime"] = now
                    ordersIP.append(thisOrder)
                    replaceTicker(item) # replace ticker when trade occurs

        elif (slope40 < 0 and slope20 < 0) or (slope40 > 0 and slope20 > 0):
            if (now > replaceTickersTime):
                replaceTicker(item)
                replaceTickersTime = now + datetime.timedelta(minutes=5)
