/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * @path ch07/7.6/7.6.1/7.6.1-4-10.js
 * @description Allow reserved words as property names by set function within an object, verified with hasOwnProperty: in, try, class
 */


function testcase() {
        var test0 = 0, test1 = 1, test2 = 2;
        var tokenCodes  = {
            set in(value){
                test0 = value;
            },
            get in(){
                return test0;
            },
            set try(value){
                test1 = value;
            },
            get try(){
                return test1
            },
            set class(value){
                test2 = value;
            },
            get class(){
                return test2;
            }
        };      
        var arr = [
            'in', 
            'try',
            'class'
            ];
        for(var p in tokenCodes) {       
            for(var p1 in arr) {                
                if(arr[p1] === p) {
                    if(!tokenCodes.hasOwnProperty(arr[p1])) {
                        return false;
                    };
                }
            }
        }
        return true;
    }
runTestCase(testcase);
