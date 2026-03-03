#!/usr/bin/python3

class DB:

    def __init__(self,Config):

        from math import floor
        from os import getcwd
        from os.path import join
        from json import loads, dumps, dump
        from datetime import timedelta, datetime, timezone 
        from pymongo import MongoClient , errors, ReturnDocument
        from urllib import parse
        from urllib.request import  urlopen 
        from bson.objectid import ObjectId  
       
        self.Config                         = Config
        self.getcwd                         = getcwd
        self.join                           = join 
        self.floor                          = floor 
        self.loads                          = loads
        self.dumps                          = dumps
        self.dump                           = dump  
        self.datetime                       = datetime
        self.ObjectId                       = ObjectId 
        self.server                         = Config.DB_SERVER
        self.port                           = Config.DB_PORT
        self.username                       = parse.quote_plus(Config.DB_USERNAME)
        self.password                       = parse.quote_plus(Config.DB_PASSWORD)
        self.remoteMongo                    = MongoClient
        self.ReturnDocument                 = ReturnDocument
        self.PyMongoError                   = errors.PyMongoError
        self.BulkWriteError                 = errors.BulkWriteError  
        self.tls                            = False

    def __del__(self):
        pass

    ####################
    # LAB 4 FUNCTIONS  #
    ####################
    
    def addRadar(self, data):
        try:
            remotedb = self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port), tls=self.tls)
            result = remotedb.ELET2415.radar.insert_one(data)
        except Exception as e:
            msg = str(e)
            if "duplicate" not in msg:
                print("addRadar error ", msg)
            return False
        else:
            return True

    def getRadarInRange(self, start, end):
        try:
            remotedb = self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port), tls=self.tls)
            result = list(remotedb.ELET2415.radar.find({"timestamp": {"$gte": int(start), "$lte": int(end)}}, {"_id": 0}))
        except Exception as e:
            print("getRadarInRange error ", str(e))
            return []
        else:
            return result

    def avgReserve(self, start, end):
        try:
            remotedb = self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port), tls=self.tls)
            result = list(remotedb.ELET2415.radar.aggregate([
                {"$match": {"timestamp": {"$gte": int(start), "$lte": int(end)}}},
                {"$group": {"_id": None, "average": {"$avg": "$reserve"}}},
                {"$project": {"_id": 0}}
            ]))
        except Exception as e:
            print("avgReserve error ", str(e))
            return []
        else:
            return result

    def setPasscode(self, passcode):
        try:
            remotedb = self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port), tls=self.tls)
            result = remotedb.ELET2415.code.find_one_and_update(
                {"type": "passcode"},
                {"$set": {"code": passcode}},
                projection={"_id": 0},
                upsert=True,
                return_document=self.ReturnDocument.AFTER
            )
        except Exception as e:
            print("setPasscode error ", str(e))
            return False
        else:
            return True

    def checkPasscode(self, passcode):
        try:
            remotedb = self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port), tls=self.tls)
            result = remotedb.ELET2415.code.count_documents({"type": "passcode", "code": passcode})
        except Exception as e:
            print("checkPasscode error ", str(e))
            return 0
        else:
            return result


def main():
    from config import Config
    from time import time, ctime, sleep
    from math import floor
    from datetime import datetime, timedelta
    one = DB(Config)

    start = time() 
    end = time()
    print(f"completed in: {end - start} seconds")
    
if __name__ == '__main__':
    main()