
int a = 0;

Thread thread = new Thread() {
    Thread thread_a = new Thread() {
        System.msleep(1);
    }

    thread_a.join();

    Thread thread2 = new Thread() {
        System.msleep(1);
    }

    Thread thread3 = new Thread() {
        System.msleep(1);
    }

    thread2.join();
    thread3.join();
}

Thread thread4 = new Thread() {
    Thread thread_b = new Thread() {
        System.msleep(1);
    }

    thread_b.join();

    Thread thread5 = new Thread() {
        System.msleep(1);
    }

    Thread thread6 = new Thread() {
        System.msleep(1);
    }

    thread5.join();
    thread6.join();
}

thread.join();
thread4.join();

